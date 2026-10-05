/*
 * NLOS scoring, one detector per tier (Physical sect 1.3 and sect 2.5; TECH_DEBT L.2.2, K.8).
 *
 * WHAT THE SPEC ASKS FOR. PCS = w1*(1/trace(Sigma)) + w2*GDOP_factor + w3*(1 - NLOS_score),
 * with NLOS_score tier-specific:
 *   UWB     compare the CIR's leading-edge amplitude to its peak amplitude,
 *           NLOS_score = 1 - A_leading_edge / A_peak                        (sect 1.3)
 *   2.4 GHz cross-check the ToF-implied distance against the RSSI-implied distance via
 *           the path-loss model and flag disagreement beyond expected noise   (sect 2.5)
 *   868 MHz flag high RSSI/SNR variance across consecutive readings on one link (sect 2.5)
 * Until this file the simulator had no CIR at all, so no classifier could exist, and the
 * PCS NLOS term read a boolean tag (--nlosSource) instead of a score.
 *
 * THE CIRCULARITY, stated where the code is, because it bounds every number this file
 * produces. Nothing here observes a physical channel. The UWB CIR is SYNTHESISED from the
 * link state the physics engine already decided -- its LOS/NLOS draw, its Rician K-factor,
 * its obstruction loss, its SNR -- and the leading-edge test then reads that synthetic CIR.
 * The 2.4 GHz and 868 MHz scores read RSSI, SNR and range values the same engine produced
 * from the same draw. So each classifier can only REDISCOVER the NLOS the channel model
 * created: a confusion matrix against the modelled truth measures how much of the engine's
 * own NLOS decision survives into the observables this file is allowed to read. It says
 * nothing about how the detector would do on a real channel, and a perfect score would
 * mean only that the synthesis encodes the label too directly.
 *
 * WHAT EACH DETECTOR MAY READ. Only what a receiver has: the CIR magnitude and its noise
 * floor (UWB); the measured RSSI, the ToF range, the configured transmit power and the
 * deployment's nominal path-loss model (2.4 GHz); the measured RSSI and SNR history of the
 * link (868 MHz); and, for the last two, the node's own recent readings on that tier as
 * the reference for "expected noise" (CaTblRobustRef). The engine's truth -- isNLOS,
 * nlosDb, K -- enters ONLY the UWB synthesis, never a detector, and the confusion matrix
 * is where the truth is compared.
 *
 * RANDOMNESS. The CIR synthesis draws from one UniformRandomVariable whose Stream is set
 * at construction, so no automatic stream index is consumed and no engine draw moves.
 * Every other score here is deterministic arithmetic.
 */

#ifndef CA_TBL_NLOS_CLASSIFIER_H
#define CA_TBL_NLOS_CLASSIFIER_H

#include "lora-physics-parameters.h"

#include "ns3/ptr.h"
#include "ns3/random-variable-stream.h"

#include <cstdint>
#include <deque>
#include <map>
#include <vector>

namespace ns3
{

/**
 * \brief One IEEE 802.15.4a channel-model parameter set.
 *
 * SOURCE: IEEE 802.15.4a channel model final report, IEEE P802.15-04-0662-02-004a
 * (Molisch et al., 2004), Appendix A MATLAB listing (uwb_sv_params_15_4a); the same model
 * is published as A. F. Molisch et al., "A Comprehensive Standardized Model for
 * Ultrawideband Propagation Channels", IEEE Trans. Antennas Propag. 54(11):3151-3166,
 * 2006. The LISTING's values are used throughout. Where the report's printed tables differ
 * -- office NLOS chi 0.86 in the table against 0.78 in the listing, outdoor LOS Lambda
 * 0.0048 against 0.0448 and lambda_1 0.27 against 0.13 -- the listing is taken, because it
 * is the generator the model's reference impulse responses were produced with and it has
 * no NA gaps.
 */
struct CaTblCirModel
{
    const char* name;
    double clusterRate;    //!< Lambda, 1/ns: cluster arrival rate (eq. 17)
    double meanClusters;   //!< L-bar: Poisson mean number of clusters (eq. 16)
    double rayRate1;       //!< lambda_1, 1/ns (eq. 18)
    double rayRate2;       //!< lambda_2, 1/ns
    double rayMix;         //!< beta: mixture probability of lambda_1
    double clusterDecayNs; //!< Gamma: inter-cluster power decay (eq. 21)
    double rayDecayNs;     //!< gamma_0: intra-cluster decay (eq. 20, k_gamma = 0 here)
    double kGamma;         //!< k_gamma: growth of gamma with cluster delay
    double clusterSigmaDb; //!< sigma_cluster: cluster shadowing (eq. 21)
    //! 0 = LOS, first cluster at the direct-path delay. 1 = NLOS whose first cluster
    //! arrives a random Exp(1/Lambda) later (the listing's `nlos == 1`). 2 = NLOS whose
    //! first cluster follows the rise-then-decay shape of eq. 22 (`nlos == 2`).
    int nlosMode;
    double gammaRiseNs; //!< eq. 22, mode 2 only
    double gamma1Ns;    //!< eq. 22, mode 2 only
    double chi;         //!< eq. 22, mode 2 only: attenuation of the first component
    double m0;          //!< Nakagami m: log-normal location (listing `m0`, natural log)
    double sigmaM0;     //!< Nakagami m: log-normal scale (listing `sigma_m0`)
    double mFirst;      //!< m of each cluster's first ray, or 0 to draw it like the rest
};

/**
 * \brief The parameter set for an environment class and the link's modelled state.
 *
 * Office (CM3 LOS / CM4 NLOS) for ENV_INDOOR_OFFICE; outdoor (CM5 LOS / CM6 NLOS) for
 * every other class. The mapping is OURS. The scenario is outdoor search and rescue, so
 * outdoor is the default; office is kept because --envFacility=6 is the one class whose
 * NLOS probability is non-negligible at UWB distances, and CM4 is the 802.15.4a set whose
 * NLOS PDP explicitly attenuates the first component. Industrial (CM7/CM8) needs the
 * dense tapped-delay-line arrival mode and residential (CM1/CM2) has no counterpart here;
 * both fall back to outdoor, a stated limit.
 */
const CaTblCirModel& CaTblCirModelFor(EnvironmentClass env, bool nlos);

/** Detector constants. EVERY ONE IS OURS, not cited; see each field. */
struct CaTblCirConfig
{
    //! Tap spacing of the synthetic accumulator, ns. ~1 ns is the order of an IEEE
    //! 802.15.4 HRP receiver's CIR accumulator spacing at 499.2 MHz; the pulse shape is
    //! NOT modelled (rays are binned, not filtered), a stated limit.
    double tapNs;
    //! Noise-only taps kept before the direct-path delay, ns, so the leading-edge search
    //! starts in noise as a real accumulator's does.
    double preNs;
    //! Window after the direct-path delay, ns. Rays beyond it are dropped.
    double windowNs;
    //! Leading-edge threshold in noise sigmas. The noise magnitude per tap is Rayleigh, so
    //! P(|n| > k*sigma) = exp(-k^2): 1.2e-4 per tap at k = 3, ~0.2% over the pre-window.
    double noiseK;

    CaTblCirConfig();
};

/** One synthesised CIR and the leading-edge test on it. */
struct CaTblCirResult
{
    bool detected;       //!< a tap reached the threshold at or before the peak
    int32_t leTap;       //!< leading-edge tap index, -1 if none
    int32_t peakTap;     //!< peak tap index
    double leDelayNs;    //!< leading edge relative to the true direct-path delay
    double peakDelayNs;  //!< peak relative to the true direct-path delay
    double aLeadingEdge; //!< |h| at the leading edge
    double aPeak;        //!< |h| at the peak
    double noiseSigma;   //!< rms complex noise per tap
    double directPower;  //!< direct-path power relative to unit diffuse energy
    double score;        //!< 1 - aLeadingEdge / aPeak, in [0,1); -1 if not detected
    uint32_t nClusters;
    uint32_t nRays;

    CaTblCirResult();
};

/**
 * \brief The leading-edge test on a CIR magnitude vector. Pure; no RNG.
 *
 * Peak = the largest |h|. Leading edge = the FIRST tap, scanning from the start of the
 * window up to the peak, whose |h| reaches noiseK * noiseSigma. NLOS_score = 1 - A_le /
 * A_peak (Physical sect 1.3). When the peak itself is below the threshold nothing is
 * detected and the score is -1: a CIR that cannot be told from noise carries no NLOS
 * evidence either way, and scoring it would invent some.
 */
CaTblCirResult CaTblLeadingEdge(const std::vector<double>& mag, double noiseSigma, double noiseK);

/**
 * \brief Synthesise a UWB CIR from a link's modelled channel state and score it.
 *
 * THE CIR, per link evaluation (ALL OF THE COMPOSITION BELOW IS OURS; the multipath
 * statistics inside it are 802.15.4a's):
 *   diffuse   one Saleh-Valenzuela realisation of the CaTblCirModelFor(env, nlos) set:
 *             Poisson cluster count, Poisson cluster arrivals, mixed-Poisson ray
 *             arrivals, exponential cluster and ray decay (eq. 19-21) or the eq. 22
 *             first-cluster shape, log-normal cluster shadowing, Nakagami-m ray fading
 *             with log-normal m, uniform ray phases (eq. 15), normalised to unit energy.
 *   direct    one ray at the true time of flight, power K * 10^(-nlosDb/10) relative to
 *             that unit diffuse energy, uniform phase. K is the Rician K-factor the engine
 *             sampled for this link -- the specular-to-scattered power ratio, which is
 *             exactly what the direct ray is against the diffuse part. nlosDb is the
 *             obstruction loss the engine drew for this link. The engine applies it to
 *             the whole narrowband link budget; resolving the channel in delay puts it on
 *             the one ray that crosses the obstruction, while the scattered energy goes
 *             round it. That attribution is our modelling choice.
 *   noise     complex Gaussian per tap, variance (1 + direct power) / SNR_acc, where
 *             SNR_acc is the link's in-band SNR plus the preamble accumulation gain
 *             10*log10(PSR) -- the same gain lora-ranging-submodule.cc gives the UWB
 *             ranging bound. That is, the accumulated SNR is read as total CIR energy over
 *             per-tap noise variance: ours.
 * Rays are binned to taps by delay and summed coherently; the pulse shape is not modelled.
 */
class CaTblCirSynth
{
  public:
    CaTblCirSynth(int64_t stream, const CaTblCirConfig& cfg);

    //! Complex taps -> magnitudes. Exposed so the unit suite can check the synthesis
    //! itself (energy, delay of the first cluster) apart from the detector.
    std::vector<double> Synthesize(const CaTblCirModel& m,
                                   double directPower,
                                   double snrAccDb,
                                   CaTblCirResult& info);

    //! Synthesise from the engine's link state and run CaTblLeadingEdge() on it.
    CaTblCirResult Classify(EnvironmentClass env,
                            bool nlosTruth,
                            double kFactor,
                            double nlosDb,
                            double snrAccDb);

    const CaTblCirConfig& GetConfig() const { return m_cfg; }

  private:
    double Uniform01(); //!< (0, 1]
    double StdNormal(); //!< Box-Muller on Uniform01, no cached spare (one draw pair each)
    double Expo(double rate);
    double GammaUnit(double shape); //!< Marsaglia-Tsang, unit scale
    uint32_t Poisson(double mean);  //!< Knuth; means here are <= 14

    Ptr<UniformRandomVariable> m_u;
    CaTblCirConfig m_cfg;
};

/**
 * \brief Deterministic mirror of ComputePathLossDb() at the parameter MEANS.
 *
 * lora-propagation-submodule.cc draws the path-loss exponent from its distribution on the
 * engine's shared sampler; a receiver has only the deployment's nominal model, and calling
 * the engine's function would also consume a draw from the unseeded default stream
 * (TECH_DEBT 1.1) and move every link in the run. So this is the same three branches
 * (3GPP UMa for the urban classes, dual-slope, single-slope) with the means, no sampler.
 * If ComputePathLossDb() changes, this must change with it -- the same contract
 * InvertPropagationLossDb() in lora-ranging-submodule.cc already carries.
 */
double CaTblNominalPathLossDb(double distanceM, const PropagationParams& prop);

/** Map a normalised disagreement z onto [0,1]: 0 at z <= zLo, 1 at z >= zHi, linear in
 *  between. zLo = 1 ("within one expected sigma is noise") and zHi = 3 are OURS. */
double CaTblNlosRamp(double z, double zLo, double zHi);

/**
 * \brief "Expected noise", learned from the node's own recent readings on one tier.
 *
 * WHY NOT FROM THE CONFIGURATION. The first build took the expected noise from the
 * engine's configured shadowing sigma and the Rician dB sigma at its LOS K-factor, and the
 * smoke run refuted both (seed 7, legacy topology, ca_tbl_full). On 868 MHz, successive
 * readings on a link whose modelled state was LOS both times differed by 15.9 dB rms --
 * 11.2 dB per reading -- against 2.9 dB predicted from fading, so 87% of links were
 * flagged. On 2.4 GHz the RSSI sat a median 11.3 dB below the nominal path-loss model at
 * the ToF distance on LOS links (p10 1.9, p90 22.9): foliage, terrain and the other terms
 * the nominal model omits. A receiver knows neither number from first principles; a
 * deployed one learns its link statistics, and so does this.
 *
 * WHAT IT IS: the median and 1.4826 * MAD (the Gaussian-consistent robust sigma) of the
 * last `pool` values, and the standard score z = (x - median) / sigma of a new value
 * against the pool BEFORE that value joins it. Median and MAD rather than mean and SD
 * because the values being flagged are themselves in the pool; a robust reference stays
 * put while a minority of NLOS readings sit in it. `pool` and `minPool` are OURS.
 *
 * WHAT IT COSTS: a detector referenced to its own population flags what is UNUSUAL, not
 * what is obstructed. Where most links are NLOS it will call them normal. That is the
 * price of not assuming a channel model the receiver does not have, and it is stated.
 */
class CaTblRobustRef
{
  public:
    CaTblRobustRef(uint32_t pool, uint32_t minPool);

    //! Standard score of \p x against the current pool; false (and z untouched) while
    //! the pool holds fewer than minPool values. Does NOT add \p x.
    bool Score(double x, double& z, double& median, double& sigma) const;
    //! Add \p x to the pool, dropping the oldest value beyond `pool`.
    void Add(double x);
    size_t Size() const { return m_vals.size(); }

  private:
    uint32_t m_pool;
    uint32_t m_minPool;
    std::deque<double> m_vals;
};

/** The 2.4 GHz ToF-vs-RSSI residual (sect 2.5). */
struct CaTblTofRssiResidual
{
    double predRssiDbm; //!< tx power - nominal path loss at the ToF distance
    double excessDb;    //!< predicted - measured RSSI: loss the ToF distance does not explain
    bool valid;         //!< false when the range or the RSSI is not usable

    CaTblTofRssiResidual();
};

/**
 * \brief How much more loss a 2.4 GHz link shows than its ToF distance predicts.
 *
 * Comparing distances and comparing RSSI at one distance are the same test: the
 * RSSI-implied distance exceeds the ToF distance by a factor 10^(excess / (10 n)). It is
 * computed in dB because that is where the noise is Gaussian. The prediction is
 * CaTblNominalPathLossDb() -- the deployment's path-loss model -- and the "expected noise"
 * the spec compares against is the tier's CaTblRobustRef over this residual, which also
 * absorbs the model's average bias. ONE-SIDED when scored: an obstruction only ever
 * removes power, so an RSSI above the prediction is noise, not NLOS evidence.
 */
CaTblTofRssiResidual CaTblTofRssiExcess(double tofRangeM,
                                        double rssiDbm,
                                        double txPowerDbm,
                                        const PropagationParams& prop);

/** The 868 MHz per-link variability (sect 2.5). */
class CaTblLinkVariability
{
  public:
    struct Result
    {
        uint32_t n;     //!< readings in the window after this one
        double sRssiDb; //!< von Neumann sigma of the RSSI readings
        double sSnrDb;  //!< von Neumann sigma of the SNR readings
        bool valid;     //!< false while the window is shorter than minReadings

        Result();
    };

    /**
     * \param window      readings kept per link. 8 is OURS.
     * \param minReadings readings before a value is given. 3 (two successive
     *                    differences) is OURS.
     *
     * VON NEUMANN, NOT THE SAMPLE VARIANCE: s^2 = sum (x_i - x_{i-1})^2 / (2 (n - 1)).
     * The node walks, so a link's mean RSSI drifts with distance; successive differences
     * cancel that drift to first order and the plain variance would read it as NLOS.
     *
     * RSSI AND SNR ARE BOTH READ, as the spec asks, and the caller uses the larger. In this
     * engine they are nearly redundant -- SNR is RSSI over a noise floor that moves by
     * hundredths of a dB -- which is stated rather than hidden by dropping one.
     *
     * THE TRUTH IT IS SCORED AGAINST IS PER READING, and the statistic is per link. The
     * engine draws LOS/NLOS independently per transmission, so a link's variability says
     * how often it toggles, not which state this reading is in. Expect a weak detector:
     * that is the spec's method meeting this channel model, not a defect to tune away.
     */
    CaTblLinkVariability(uint32_t window, uint32_t minReadings);

    Result Push(uint32_t linkKey, double rssiDbm, double snrDb);

  private:
    uint32_t m_window;
    uint32_t m_minReadings;
    std::map<uint32_t, std::deque<std::pair<double, double>>> m_hist;
};

} // namespace ns3

#endif // CA_TBL_NLOS_CLASSIFIER_H

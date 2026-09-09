/*
 * CA-TBL ch10 sect 10.6.5 -- CAL-01: PCS weight determination by leave-one-out
 * cross-validation.
 *
 * WHAT THIS IS AND IS NOT. The blueprint specifies that w1, w2, w3 are FITTED by LOOCV
 * against realised error. They never were: the values in ca-tbl-policy are HAND-SET, the
 * header says so, and TECH_DEBT K.8 records that omission as the direct cause of the
 * saturation defect A8 closed by result. This supplies the fitting machinery. It does NOT
 * supply the calibration, because a calibration is a measurement campaign against logged
 * data and nobody has run one.
 *
 * LOOCV RATHER THAN A TRAIN/TEST SPLIT, and the spec's reason is the honest one: with
 * three gateways, five field nodes and a six-week collection window there is not enough
 * data for a held-out set that means anything. Leave-one-out uses every sample for
 * validation exactly once, which is the standard answer for small calibration sets and is
 * cited rather than invented.
 *
 * READ K.8 BEFORE USING THIS. The measurement says PCS re-encodes the EKF covariance
 * trace and adds nothing the trace did not already carry -- every form loses the monotone
 * control with the whole 95% CI below zero. FITTING BETTER WEIGHTS DOES NOT FIX THAT. The
 * best a refit can do is bring the score back up to the control it currently loses to,
 * because two of its three terms have structurally dead inputs (hopWeight is 0, and the
 * NLOS term is pinned by an operative tag firing on 75-78% of links). Calibrating a
 * combination of one live term and two dead ones optimises the live one and relabels it.
 *
 * So the order K.8 gives matters: run the monotone control FIRST as an acceptance test,
 * then this, then give the hop and NLOS terms real inputs. Running this first would
 * produce fitted weights for a score that is still measured to carry no information, and
 * the fit would look like progress.
 */

#ifndef CA_TBL_LOOCV_H
#define CA_TBL_LOOCV_H

#include <cstdint>
#include <vector>

namespace ns3
{

//! One calibration sample: the three PCS terms as computed, and the error actually
//! realised on that cycle. The target is the ERROR, not the score -- fitting the score
//! to itself is the circularity this whole exercise exists to avoid.
struct CaTblCalSample
{
    double term1 = 0.0;  //!< certainty from the filter, reference/(reference + trace)
    double term2 = 0.0;  //!< geometry spread
    double term3 = 0.0;  //!< clear-path fraction
    double realisedErrorM = 0.0;
};

enum CaTblCalVerdict : uint8_t
{
    CA_TBL_CAL_OK = 0,
    CA_TBL_CAL_TOO_FEW_SAMPLES,   //!< fewer samples than free parameters plus one
    CA_TBL_CAL_DEGENERATE,        //!< a term is constant, so its weight is unidentifiable
    CA_TBL_CAL_NO_IMPROVEMENT     //!< the fit does not beat the covariance trace alone
};

struct CaTblCalResult
{
    double w1 = 0.0;
    double w2 = 0.0;
    double w3 = 0.0;
    //! Mean LOOCV prediction error of the fitted weights.
    double loocvRmse = -1.0;
    /** THE ACCEPTANCE TEST, and it is the point of the struct. Mean LOOCV error of the
     *  trivial control -- term 1 alone, which is a monotone function of the covariance
     *  trace the score is computed from. A fit that does not beat this has not earned
     *  its two extra parameters, and reporting the fitted weights without it is how a
     *  re-encoding gets published as a model. */
    double controlRmse = -1.0;
    uint32_t samplesUsed = 0;
    CaTblCalVerdict verdict = CA_TBL_CAL_TOO_FEW_SAMPLES;
};

/**
 * \brief Fit w1..w3 by leave-one-out cross-validation against realised error.
 *
 * \param samples    calibration set
 * \param requireImprovement  if true, a fit that does not beat the term-1-only control
 *        returns CA_TBL_CAL_NO_IMPROVEMENT and the weights are NOT recommended. Default
 *        true, because the alternative is a function that always produces an answer.
 *
 * Weights are normalised so they sum to the number of terms, keeping them on the same
 * scale as the hand-set values and the thresholds those were chosen against. A fit that
 * silently rescaled would move every escalation threshold with it.
 */
CaTblCalResult CaTblCalibrateLoocv(const std::vector<CaTblCalSample>& samples,
                                   bool requireImprovement = true);

const char* CaTblCalVerdictName(CaTblCalVerdict v);

} // namespace ns3

#endif // CA_TBL_LOOCV_H

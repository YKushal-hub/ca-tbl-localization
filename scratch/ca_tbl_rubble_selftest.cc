/*
 * Rubble gate (G177) unit program: prints the World rubble model's values as
 * key=value lines. scripts/rubble_gate.py compares each against the row in
 * docs/SOURCES_COLLAPSE.md and against the A5 validation targets, and checks
 * the geometry fixtures and the refusals (negative fixtures).
 */
#include "ns3/ca-tbl-rubble-world.h"
#include "ns3/core-module.h"

#include <cstdio>

using namespace ns3;

static void
P(const char* k, double v)
{
    std::printf("%s=%.6f\n", k, v);
}

int
main(int argc, char** argv)
{
    CommandLine cmd;
    cmd.Parse(argc, argv);
    const char* bn[3] = {"low", "central", "high"};
    const double f[6] = {0.868e9, 1.0e9, 2.0e9, 2.4e9, 6.5e9, 8.0e9};
    const char* fn[6] = {"868", "1000", "2000", "2400", "6500", "8000"};
    P("itu_1000", CaTblRubbleWorld::ItuConcreteDbPerM(1.0e9));
    P("itu_2000", CaTblRubbleWorld::ItuConcreteDbPerM(2.0e9));
    P("itu_2400", CaTblRubbleWorld::ItuConcreteDbPerM(2.4e9));
    P("itu_6500", CaTblRubbleWorld::ItuConcreteDbPerM(6.5e9));
    P("itu_8000", CaTblRubbleWorld::ItuConcreteDbPerM(8.0e9));
    for (int b = 0; b < 3; ++b)
    {
        CaTblRubbleWorld w(static_cast<CaTblRubbleBracket>(b));
        char k[64];
        std::snprintf(k, sizeof k, "eta_%s", bn[b]);
        P(k, CaTblRubbleWorld::Eta(static_cast<CaTblRubbleBracket>(b)));
        std::snprintf(k, sizeof k, "bias_%s", bn[b]);
        P(k, w.BiasPerM());
        for (int i = 0; i < 6; ++i)
        {
            std::snprintf(k, sizeof k, "A_%s_%s", bn[b], fn[i]);
            P(k, w.SpecificLossDbPerM(f[i]));
        }
        // engine centre frequencies
        std::snprintf(k, sizeof k, "A_%s_2440", bn[b]);
        P(k, w.SpecificLossDbPerM(2.44e9));
        std::snprintf(k, sizeof k, "A_%s_7990", bn[b]);
        P(k, w.SpecificLossDbPerM(7.99e9));
    }

    // Geometry fixtures. A 10 x 10 x 4 m pile with a 2 x 2 x 1 m void.
    CaTblRubbleWorld g(CA_TBL_RUBBLE_CENTRAL);
    P("add_pile", g.AddRubble(Vector(0, 0, 0), Vector(10, 10, 4)) ? 1 : 0);
    P("add_void", g.AddVoid(Vector(4, 4, 1), Vector(6, 6, 2)) ? 1 : 0);
    // vertical line straight through the void: 4 m of pile minus 1 m of void = 3
    P("geo_vert_void", g.PileMetres(Vector(5, 5, -1), Vector(5, 5, 10)));
    // vertical line missing the void: 4
    P("geo_vert", g.PileMetres(Vector(1, 1, -1), Vector(1, 1, 10)));
    // segment ending inside the pile: from z = 10 down to z = 3 -> 1 m
    P("geo_partial", g.PileMetres(Vector(1, 1, 10), Vector(1, 1, 3)));
    // diagonal 3-4-5 in the x-z plane entirely inside: length 5
    P("geo_diag", g.PileMetres(Vector(1, 1, 0), Vector(4, 1, 4)));
    // outside entirely
    P("geo_out", g.PileMetres(Vector(20, 20, 0), Vector(30, 20, 0)));
    // symmetric
    P("geo_sym", g.PileMetres(Vector(5, 5, 10), Vector(5, 5, -1)));
    P("in_void", g.InRubble(Vector(5, 5, 1.5)) ? 1 : 0);
    P("in_pile", g.InRubble(Vector(1, 1, 1)) ? 1 : 0);
    // a buried device 2 m deep at (1,1,2) to a rescuer at (1,1,5): 2 m of pile
    P("loss_2m_868", g.LossDb(0.868e9, Vector(1, 1, 2), Vector(1, 1, 5)));
    P("bias_2m", g.BiasM(Vector(1, 1, 2), Vector(1, 1, 5)));

    // Negative fixtures: each must be refused (prints 0).
    P("neg_overlap", g.AddRubble(Vector(9, 9, 0), Vector(12, 12, 4)) ? 1 : 0);
    P("neg_degenerate", g.AddRubble(Vector(20, 20, 0), Vector(20, 25, 4)) ? 1 : 0);
    P("neg_void_outside", g.AddVoid(Vector(9, 9, 3), Vector(11, 11, 3.5)) ? 1 : 0);
    P("neg_void_overlap", g.AddVoid(Vector(5, 5, 1.5), Vector(7, 7, 2.5)) ? 1 : 0);
    CaTblRubbleBracket br;
    P("neg_parse", ParseRubbleBracket("medium", br) ? 1 : 0);
    P("pos_parse", ParseRubbleBracket("high", br) && br == CA_TBL_RUBBLE_HIGH ? 1 : 0);
    return 0;
}

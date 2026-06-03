import sys
from plotters import heat_conduction_reference_example_plotter as hcrep
from plotters.three_body_plotter import ThreeBodyPlotter

if __name__ == '__main__':
    if len(sys.argv) != 4:
        print("Usage: python plot.py PlotterName input.json output.mp4")
        sys.exit(1)

    plotters = {
        "HeatConductionReferenceExamplePlotter":
            hcrep.HeatConductionReferenceExamplePlotter,
        "ThreeBodyPlotter": ThreeBodyPlotter
    }

    if sys.argv[1] not in plotters:
        raise SystemError("Unknown plotter")

    Plotter = plotters[sys.argv[1]]
    plotter = Plotter(sys.argv[2], sys.argv[3])
    plotter.plot()

within ControlLawDiagrams.F16Block25;
model LongitudinalGainsVerification
  "Numeric breakpoint checks for the source-defined Figure 3.2 functions"
  import G = ControlLawDiagrams.F16Block25.LongitudinalGains;
  constant Real TOLERANCE = 1e-9;
  Real verificationMarker;
initial algorithm
  assert(abs(G.pitchCommandGradientValue(-30.25) + 10.86) < TOLERANCE,
    "Pitch-command gradient forward endpoint is incorrect");
  assert(abs(G.pitchCommandGradientValue(-4.25) + 0.44) < TOLERANCE,
    "Pitch-command gradient forward knee is incorrect");
  assert(abs(G.pitchCommandGradientValue(4.25) - 0.44) < TOLERANCE,
    "Pitch-command gradient aft knee is incorrect");
  assert(abs(G.pitchCommandGradientValue(30.25) - 10.86) < TOLERANCE,
    "Pitch-command gradient aft endpoint is incorrect");
  assert(abs(G.n3Value(250.0) - 1.0) < TOLERANCE,
    "N3 low-pressure plateau is incorrect");
  assert(abs(G.n3Value(850.0) - 0.555) < TOLERANCE,
    "N3 middle breakpoint is incorrect");
  assert(abs(G.n3Value(550.0) - 0.7775) < TOLERANCE,
    "N3 linear interpolation is incorrect");
  assert(abs(G.n3Value(2970.0) - 0.086) < TOLERANCE,
    "N3 high-pressure endpoint is incorrect");
  assert(abs(G.n3aValue(250.0)) < TOLERANCE,
    "N3A zero plateau is incorrect");
  assert(abs(G.n3aValue(600.0) - 0.30) < TOLERANCE,
    "N3A inferred peak is incorrect");
  assert(abs(G.n3aValue(2970.0)) < TOLERANCE,
    "N3A high-pressure return is incorrect");
  assert(abs(G.n4Value(100.0, true) - 0.70) < TOLERANCE,
    "N4 Note 1 branch is incorrect");
  assert(abs(G.n4Value(100.0, false) - 0.63) < TOLERANCE,
    "N4 default branch is incorrect");
  assert(abs(G.n7Value(400.0, true) - 0.14) < TOLERANCE,
    "N7 minimum gain is incorrect");
  assert(abs(G.n14Value(260.0) - 8.3) < TOLERANCE,
    "N14 low-pressure pole is incorrect");
  assert(abs(G.n14Value(1400.0) - 6.0) < TOLERANCE,
    "N14 high-pressure pole is incorrect");
  assert(abs(G.n14Value(830.0) - 7.15) < TOLERANCE,
    "N14 linear interpolation is incorrect");
  assert(abs(G.f1Value(34.0) + 1.0) < TOLERANCE,
    "F1 low-pressure limit is incorrect");
  assert(abs(G.f1Value(184.0) + 4.0) < TOLERANCE,
    "F1 high-pressure limit is incorrect");
  assert(abs(G.f2Value(795.0, 1500.0) - 0.5) < TOLERANCE,
    "F2 plateau is incorrect");
  assert(abs(G.f2Value(2685.0, 1500.0) + 1.0) < TOLERANCE,
    "F2 high-ratio endpoint is incorrect");
  assert(abs(G.rollAlphaIncrementValue(8.4, false) - 5.4) < TOLERANCE,
    "CAT I roll-rate alpha increment is incorrect");
  assert(abs(G.rollAlphaIncrementValue(8.4, true) - 2.7) < TOLERANCE,
    "Reduced-authority roll-rate alpha increment is incorrect");
  assert(abs(G.n8Value(1086.0, 2000.0, false) - 17.0) < TOLERANCE,
    "CAT I N8 breakpoint is incorrect");
  assert(abs(G.n8Value(1086.0, 2000.0, true) - 16.2) < TOLERANCE,
    "CAT III N8 breakpoint is incorrect");
equation
  verificationMarker = time;
end LongitudinalGainsVerification;

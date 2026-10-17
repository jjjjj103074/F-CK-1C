within ControlLawDiagrams.F16Block25;
model LongitudinalControlVerification
  "Executable boundary harness for every Figure 3.1 signal path"
  constant Real OUTPUT_SANITY_LIMIT = 1000.0;
  constant Real TOLERANCE = 1e-9;
  LongitudinalControl law;
  Real verificationMarker;
equation
  law.pitchTrim = 0.15 * sin(0.7 * time);
  law.forwardStickForce = if time < 0.25 then 0.0
    else if time < 0.75 then 8.0 else 0.0;
  law.aftStickForce = if time < 1.0 then 0.0
    else if time < 1.4 then -8.0 else 0.0;
  law.angleOfAttack = if time >= 0.35 and time < 0.75 then 32.0
    else 7.0 + 3.0 * sin(0.8 * time);
  law.rollRateFromRollAxis = 20.0 * sin(1.1 * time);
  law.pitchRate = 4.0 * sin(1.3 * time);
  law.normalLoadFactor = 1.0 + 0.25 * sin(0.9 * time);
  law.pitchAutopilot = if time < 1.2 then 0.0 else 0.2;
  law.leadingEdgePitchAoa = law.angleOfAttack;
  law.impactPressurePsf = 500.0;
  law.staticPressurePsf = 1500.0;
  law.n6GainValue = 0.0;
  law.standbyPitchGain = 0.30;
  law.trailingEdgeM9 = 0.50;
  law.trailingEdgeM10 = 0.25;
  law.leadingEdgeM11 = 0.10;
  law.leadingEdgeM12 = 0.0;
  law.leadingEdgeOverrideCommand = 5.0;
  law.mode1Active = if time >= 1.70 then 1.0 else 0.0;
  law.mode2Active = if time >= 0.30 and time < 0.55 then 1.0 else 0.0;
  law.mode3Active = if time >= 0.55 and time < 0.80 then 1.0 else 0.0;
  law.mode4Active = if time >= 0.80 and time < 1.05 then 1.0 else 0.0;
  law.mode5Active = if time >= 1.50 then 1.0 else 0.0;
  law.mode7Active = if time >= 1.05 and time < 1.30 then 1.0 else 0.0;
  law.mode12Active = if time >= 1.30 and time < 1.45 then 1.0 else 0.0;
  law.mode13Active = if time >= 1.45 and time < 1.60 then 1.0 else 0.0;
  law.mode14Active = if time >= 1.60 and time < 1.75 then 1.0 else 0.0;
  law.mode17Active = if time >= 1.75 then 1.0 else 0.0;
  law.note1Active = if time >= 1.0 then 1.0 else 0.0;
  law.cat3Active = if time >= 1.25 then 1.0 else 0.0;
  law.leadingEdgeOverrideActive = if time >= 1.85 then 1.0 else 0.0;
  verificationMarker = law.aaToControlSurfaceMixer +
    law.bdToControlSurfaceMixer + law.ffToControlSurfaceMixer;
  assert(abs(law.aaToControlSurfaceMixer) < OUTPUT_SANITY_LIMIT,
    "Elevator output is non-finite or divergent");
  assert(abs(law.bdToControlSurfaceMixer) < OUTPUT_SANITY_LIMIT,
    "Trailing-edge output is non-finite or divergent");
  assert(law.leadingEdgeOverrideActive > 0.5 or
    (law.ffToControlSurfaceMixer >= 0.0 and law.ffToControlSurfaceMixer <= 25.0),
    "Normal leading-edge output violated the Figure 3.1 limiter");
  assert(law.leadingEdgeOverrideActive <= 0.5 or
    abs(law.ffToControlSurfaceMixer - law.leadingEdgeOverrideCommand) < TOLERANCE,
    "Leading-edge override did not reach the output selector");
  when time >= 0.40 then
    assert(abs(law.mode2PrefilterSelector.y - law.stickCommandLimiter.y) < TOLERANCE,
      "MODE 2 did not select the Figure 3.1 bypass branch");
  end when;
  when time >= 0.65 then
    assert(law.highAlphaPositive.y > 0.0,
      "High-alpha command path was not exercised");
    assert(abs(law.mode3StructureSelector.y - law.u010PitchStructureFilter.y) < TOLERANCE,
      "MODE 3 did not select the structural-filter branch");
  end when;
  when time >= 0.90 then
    assert(abs(law.aoaMode4Selector.y) < TOLERANCE,
      "MODE 4 did not remove the alpha-limiter command");
    assert(abs(law.mode4GainSelector.y - law.u039StandbyGain.y) < TOLERANCE,
      "MODE 4 did not select standby gain");
  end when;
  when time >= 1.15 then
    assert(abs(law.mode7FeedbackSelector.y) < TOLERANCE,
      "MODE 7 did not remove integrator feedback");
  end when;
  when time >= 1.35 then
    assert(abs(law.forwardBreakoutSelector.y) < TOLERANCE,
      "MODE 12 did not zero the forward-stick sensor path");
  end when;
  when time >= 1.50 then
    assert(abs(law.aftBreakoutSelector.y) < TOLERANCE,
      "MODE 13 did not zero the aft-stick sensor path");
    assert(abs(law.bdToControlSurfaceMixer - 2.25) < TOLERANCE,
      "MODE 5 did not select M10 in the trailing-edge path");
  end when;
  when time >= 1.65 then
    assert(abs(law.mode14LowerLimit.y + 4.0) < TOLERANCE,
      "MODE 14 did not select the -4 g lower limit");
  end when;
  when time >= 1.80 then
    assert(abs(law.mode17UpperLimit.y - 4.0) < TOLERANCE,
      "MODE 17 did not select the 4 g upper limit");
  end when;
  when terminal() then
    assert(time >= 2.0, "Verification stopped before all mode branches ran");
  end when;
end LongitudinalControlVerification;

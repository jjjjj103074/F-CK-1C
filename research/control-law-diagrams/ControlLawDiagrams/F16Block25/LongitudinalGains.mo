within ControlLawDiagrams.F16Block25;
package LongitudinalGains
  "Source-defined gain functions from ADA189675 Figure 3.2"
  import T = ControlLawDiagrams.Transcription;

  constant Real PITCH_FORCE_LBF[5] = {-30.25, -4.25, 0.0, 4.25, 30.25};
  constant Real PITCH_COMMAND_G[5] = {-10.86, -0.44, 0.0, 0.44, 10.86};
  constant Real N3_IMPACT_PRESSURE_PSF[4] = {0.0, 250.0, 850.0, 2970.0};
  constant Real N3_GAIN[4] = {1.0, 1.0, 0.555, 0.086};
  constant Real N3A_IMPACT_PRESSURE_PSF[4] = {0.0, 250.0, 600.0, 2970.0}
    "Scan-derived breakpoints; the public Figure 3.2 scan does not preserve exact labels";
  constant Real N3A_GAIN[4] = {0.0, 0.0, 0.30, 0.0}
    "Curve shape and 0.30 standby value are source-visible; middle pressures are inferred";
  constant Real N4_IMPACT_PRESSURE_PSF[4] = {0.0, 100.0, 220.0, 400.0};
  constant Real N4A_GAIN[4] = {0.70, 0.70, 0.25, 0.0};
  constant Real N4B_NOTE1 = 0.0;
  constant Real N4B_DEFAULT = 0.07;
  constant Real N7_MINIMUM_GAIN = 0.14;
  constant Real N14_IMPACT_PRESSURE_PSF[2] = {260.0, 1400.0};
  constant Real N14_POLE_PER_SECOND[2] = {8.3, 6.0};
  constant Real F1_IMPACT_PRESSURE_PSF[2] = {34.0, 184.0};
  constant Real F1_NEGATIVE_COMMAND_G[2] = {-1.0, -4.0};
  constant Real F2_QC_OVER_PS[3] = {0.0, 0.53, 1.79};
  constant Real F2_STATIC_STABILITY_GAIN[3] = {0.5, 0.5, -1.0};
  constant Real ROLL_ALPHA_INPUT_DEG_PER_SECOND[2] = {3.0, 8.4};
  constant Real ROLL_ALPHA_INCREMENT_DEG[2] = {0.0, 5.4};
  constant Real ROLL_ALPHA_CAT1_LIMIT_DEG = 5.4;
  constant Real ROLL_ALPHA_CAT3_LIMIT_DEG = 2.7;
  constant Real N8_IMPACT_PRESSURE_PSF[6] = {0.0, 611.0, 897.0, 1086.0, 1403.0, 1616.0};
  constant Real N8_CAT1_PER_SECOND[6] = {7.4, 14.9, 16.2, 17.0, 21.1, 21.1};
  constant Real N8_CAT3_PER_SECOND[6] = {7.4, 14.9, 16.2, 16.2, 16.2, 16.2};
  constant Real N8_QC_OVER_PS[2] = {0.9, 1.4};
  constant Real N8_RATIO_CORRECTION[2] = {0.0, 4.0};

  function interpolateHeld
    "Piecewise-linear interpolation with the Figure 3.2 end values held"
    input Real x;
    input Real xPoints[:];
    input Real yPoints[size(xPoints, 1)];
    output Real y;
  protected
    Integer segment;
    Integer pointCount;
  algorithm
    pointCount := size(xPoints, 1);
    assert(pointCount >= 2, "A gain schedule requires at least two points");
    for point in 2:pointCount loop
      assert(xPoints[point] > xPoints[point - 1],
        "Gain-schedule breakpoints must be strictly increasing");
    end for;
    if x <= xPoints[1] then
      y := yPoints[1];
    elseif x >= xPoints[pointCount] then
      y := yPoints[pointCount];
    else
      segment := 1;
      while x > xPoints[segment + 1] loop
        segment := segment + 1;
      end while;
      y := yPoints[segment] +
        (x - xPoints[segment]) *
        (yPoints[segment + 1] - yPoints[segment]) /
        (xPoints[segment + 1] - xPoints[segment]);
    end if;
  end interpolateHeld;

  function pitchCommandGradientValue
    "Figure 3.2 pitch-command gradient after the Figure 3.1 breakout"
    input Real stickForceAfterBreakoutLbf;
    output Real commandG;
  algorithm
    commandG := interpolateHeld(
      stickForceAfterBreakoutLbf,
      PITCH_FORCE_LBF,
      PITCH_COMMAND_G);
  end pitchCommandGradientValue;

  function n3Value
    "Figure 3.2 N3 pitch-loop gain"
    input Real impactPressurePsf;
    output Real gain;
  algorithm
    gain := interpolateHeld(impactPressurePsf, N3_IMPACT_PRESSURE_PSF, N3_GAIN);
  end n3Value;

  function n4Value
    "Figure 3.2 N4 low-alpha bias-path gain"
    input Real impactPressurePsf;
    input Boolean note1Active;
    output Real gain;
  protected
    Real n4a;
    Real n4b;
  algorithm
    n4a := interpolateHeld(impactPressurePsf, N4_IMPACT_PRESSURE_PSF, N4A_GAIN);
    n4b := if note1Active then N4B_NOTE1 else N4B_DEFAULT;
    gain := n4a - n4b;
  end n4Value;

  function n3aValue
    "Figure 3.2 N3A curve reconstructed from the readable graph shape"
    input Real impactPressurePsf;
    output Real gain;
  algorithm
    gain := interpolateHeld(impactPressurePsf, N3A_IMPACT_PRESSURE_PSF, N3A_GAIN);
  end n3aValue;

  function n7Value
    "Figure 3.2 N7 high-alpha bias-path gain"
    input Real impactPressurePsf;
    input Boolean note1Active;
    output Real gain;
  algorithm
    gain := max(n4Value(impactPressurePsf, note1Active), N7_MINIMUM_GAIN);
  end n7Value;

  function n14Value
    "Figure 3.2 N14 pitch-command-lag pole"
    input Real impactPressurePsf;
    output Real polePerSecond;
  algorithm
    polePerSecond := interpolateHeld(
      impactPressurePsf,
      N14_IMPACT_PRESSURE_PSF,
      N14_POLE_PER_SECOND);
  end n14Value;

  function f1Value
    "Figure 3.2 F1 negative pitch-command limit"
    input Real impactPressurePsf;
    output Real commandG;
  algorithm
    commandG := interpolateHeld(
      impactPressurePsf,
      F1_IMPACT_PRESSURE_PSF,
      F1_NEGATIVE_COMMAND_G);
  end f1Value;

  function f2Value
    "Figure 3.2 F2 static-stability gain"
    input Real impactPressurePsf;
    input Real staticPressurePsf;
    output Real gain;
  protected
    Real pressureRatio;
  algorithm
    assert(staticPressurePsf > 0.0, "F2 requires positive static pressure");
    pressureRatio := impactPressurePsf / staticPressurePsf;
    gain := interpolateHeld(pressureRatio, F2_QC_OVER_PS, F2_STATIC_STABILITY_GAIN);
  end f2Value;

  function rollAlphaIncrementValue
    "Figure 3.1 roll-rate contribution F to the high-alpha limiter"
    input Real laggedScaledRollRate;
    input Boolean reducedAuthority;
    output Real incrementDeg;
  protected
    Real scheduled;
    Real upperLimit;
  algorithm
    scheduled := interpolateHeld(
      laggedScaledRollRate,
      ROLL_ALPHA_INPUT_DEG_PER_SECOND,
      ROLL_ALPHA_INCREMENT_DEG);
    upperLimit := if reducedAuthority then ROLL_ALPHA_CAT3_LIMIT_DEG
      else ROLL_ALPHA_CAT1_LIMIT_DEG;
    incrementDeg := min(max(scheduled, 0.0), upperLimit);
  end rollAlphaIncrementValue;

  function n8Value
    "Figure 3.2 N8 structural-filter pole"
    input Real impactPressurePsf;
    input Real staticPressurePsf;
    input Boolean cat3Active;
    output Real polePerSecond;
  protected
    Real basePole;
    Real ratioCorrection;
  algorithm
    assert(staticPressurePsf > 0.0, "N8 requires positive static pressure");
    basePole := interpolateHeld(
      impactPressurePsf,
      N8_IMPACT_PRESSURE_PSF,
      if cat3Active then N8_CAT3_PER_SECOND else N8_CAT1_PER_SECOND);
    ratioCorrection := interpolateHeld(
      impactPressurePsf / staticPressurePsf,
      N8_QC_OVER_PS,
      N8_RATIO_CORRECTION);
    polePerSecond := basePole - ratioCorrection;
    assert(polePerSecond > 0.0, "N8 schedule produced a non-positive pole");
  end n8Value;

  model PitchCommandGradient
    T.SignalInput u annotation(Placement(transformation(extent={{-110,-10},{-90,10}})));
    T.SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  equation
    y = pitchCommandGradientValue(u);
    annotation(Icon(graphics={
      Rectangle(extent={{-100,65},{100,-65}}, lineColor={0,0,0},
        fillColor={255,255,255}, fillPattern=FillPattern.Solid),
      Text(extent={{-92,50},{92,-50}},
        textString="PITCH COMMAND\nGRADIENT", fontSize=6)}));
  end PitchCommandGradient;

  model N3ScheduledGain
    "Figure 3.2 N3 schedule applied at Figure 3.1 location U-031"
    T.SignalInput impactPressurePsf
      annotation(Placement(transformation(extent={{-110,-10},{-90,10}})));
    T.SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  equation
    y = n3Value(impactPressurePsf);
    annotation(
      Icon(graphics={
        Rectangle(extent={{-100,65},{100,-65}},lineColor={0,0,0},
          fillColor={255,255,255},fillPattern=FillPattern.Solid),
        Text(extent={{-92,40},{92,-40}},textString="N3",fontSize=8)}),
      Diagram(graphics={
        Text(extent={{-90,92},{90,72}},textString="N3  PITCH LOOP GAIN",fontSize=7),
        Line(points={{-80,-35},{85,-35}},color={0,0,0},thickness=1.5,
          arrow={Arrow.None,Arrow.Filled},arrowSize=4),
        Line(points={{-80,-35},{-80,70}},color={0,0,0},thickness=1.5,
          arrow={Arrow.None,Arrow.Filled},arrowSize=4),
        Line(points={{-80,60},{-65,60},{-35,20},{75,-20}},color={0,0,0},thickness=1.5),
        Text(extent={{-97,68},{-82,52}},textString="1.0",fontSize=5),
        Text(extent={{-96,28},{-82,12}},textString="0.555",fontSize=5),
        Text(extent={{-96,-13},{-82,-29}},textString="0.086",fontSize=5),
        Text(extent={{-87,-39},{-73,-54}},textString="0",fontSize=5),
        Text(extent={{-73,-39},{-56,-54}},textString="250",fontSize=5),
        Text(extent={{-44,-39},{-26,-54}},textString="850",fontSize=5),
        Text(extent={{64,-39},{86,-54}},textString="2970",fontSize=5),
        Text(extent={{68,-58},{88,-72}},textString="q_c",fontSize=6)}));
  end N3ScheduledGain;

  model N3AScheduledGain
    "Figure 3.2 N3A schedule at U-037; unreadable pressure labels are explicit in the constants"
    T.SignalInput impactPressurePsf annotation(Placement(transformation(
      origin={100,0}, extent={{-10,-10},{10,10}}, rotation=180)));
    T.SignalOutput y annotation(Placement(transformation(
      origin={-100,0}, extent={{-10,-10},{10,10}}, rotation=180)));
  equation
    y = n3aValue(impactPressurePsf);
    annotation(
      Icon(graphics={
        Rectangle(extent={{-100,65},{100,-65}},lineColor={0,0,0},
          fillColor={255,255,255},fillPattern=FillPattern.Solid),
        Text(extent={{-92,40},{92,-40}},textString="N3A",fontSize=8)}),
      Diagram(graphics={
        Text(extent={{-90,92},{90,72}},textString="N3A  PA PITCH GAIN",fontSize=7),
        Line(points={{-80,-25},{85,-25}},color={0,0,0},thickness=1.5,
          arrow={Arrow.None,Arrow.Filled},arrowSize=4),
        Line(points={{-80,-25},{-80,70}},color={0,0,0},thickness=1.5,
          arrow={Arrow.None,Arrow.Filled},arrowSize=4),
        Line(points={{-80,-10},{-60,-10},{-25,55},{70,-10},{82,-10}},
          color={0,0,0},thickness=1.5),
        Text(extent={{-96,-3},{-82,-18}},textString="0.0",fontSize=5),
        Text(extent={{-72,-28},{-48,-43}},textString="250*",textColor={180,0,0},fontSize=5),
        Text(extent={{-40,-28},{-12,-43}},textString="600*",textColor={180,0,0},fontSize=5),
        Text(extent={{-72,66},{-42,50}},textString="0.30",fontSize=5),
        Text(extent={{55,-28},{85,-43}},textString="2970",fontSize=5),
        Text(extent={{-87,-28},{-73,-43}},textString="0",fontSize=5),
        Text(extent={{68,-47},{88,-61}},textString="q_c",fontSize=6)}));
  end N3AScheduledGain;

  model F1NegativeCommandLimit
    T.SignalInput impactPressurePsf
      annotation(Placement(transformation(extent={{-110,-10},{-90,10}})));
    T.SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  equation
    y = f1Value(impactPressurePsf);
    annotation(Icon(graphics={
      Rectangle(extent={{-100,65},{100,-65}},lineColor={0,0,0},
        fillColor={255,255,255},fillPattern=FillPattern.Solid),
      Text(extent={{-92,40},{92,-40}},textString="F1",fontSize=8)}));
  end F1NegativeCommandLimit;

  model F2StaticStabilityGain
    T.SignalInput u annotation(Placement(transformation(extent={{-110,-10},{-90,10}})));
    T.SignalInput impactPressurePsf
      annotation(Placement(transformation(extent={{-10,-110},{10,-90}})));
    T.SignalInput staticPressurePsf
      annotation(Placement(transformation(extent={{-10,90},{10,110}})));
    T.SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  equation
    y = u * f2Value(impactPressurePsf, staticPressurePsf);
    annotation(Icon(graphics={
      Rectangle(extent={{-100,65},{100,-65}},lineColor={0,0,0},
        fillColor={255,255,255},fillPattern=FillPattern.Solid),
      Text(extent={{-92,40},{92,-40}},textString="F2",fontSize=8)}));
  end F2StaticStabilityGain;

  model RollAlphaIncrement
    T.SignalInput u annotation(Placement(transformation(extent={{-110,-10},{-90,10}})));
    T.ModeInput cat3Active
      annotation(Placement(transformation(extent={{-10,90},{10,110}})));
    T.ModeInput mode1Active
      annotation(Placement(transformation(extent={{-10,-110},{10,-90}})));
    T.SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  equation
    y = rollAlphaIncrementValue(u, cat3Active > 0.5 or mode1Active > 0.5);
    annotation(Icon(graphics={
      Rectangle(extent={{-100,65},{100,-65}},lineColor={0,0,0},
        fillColor={255,255,255},fillPattern=FillPattern.Solid),
      Text(extent={{-92,42},{92,-42}},textString="F\nROLL / ALPHA",fontSize=6)}));
  end RollAlphaIncrement;

  model N8Schedule
    T.SignalInput impactPressurePsf
      annotation(Placement(transformation(extent={{-110,-10},{-90,10}})));
    T.SignalInput staticPressurePsf
      annotation(Placement(transformation(extent={{-10,-110},{10,-90}})));
    T.ModeInput cat3Active
      annotation(Placement(transformation(extent={{-10,90},{10,110}})));
    T.SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  equation
    y = n8Value(impactPressurePsf, staticPressurePsf, cat3Active > 0.5);
    annotation(Icon(graphics={
      Rectangle(extent={{-100,65},{100,-65}},lineColor={0,0,0},
        fillColor={255,255,255},fillPattern=FillPattern.Solid),
      Text(extent={{-92,40},{92,-40}},textString="N8",fontSize=8)}));
  end N8Schedule;

  model N14PitchCommandLag
    "N14/(s+N14), with N14 scheduled by impact pressure q_c"
    Real state(start=0.0, fixed=true);
    Real polePerSecond;
    T.SignalInput u annotation(Placement(transformation(extent={{-110,-10},{-90,10}})));
    T.SignalInput impactPressurePsf
      annotation(Placement(transformation(extent={{-10,-110},{10,-90}})));
    T.SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  equation
    polePerSecond = n14Value(impactPressurePsf);
    der(state) = polePerSecond * (u - state);
    y = state;
    annotation(Icon(graphics={
      Rectangle(extent={{-100,65},{100,-65}}, lineColor={0,0,0},
        fillColor={255,255,255}, fillPattern=FillPattern.Solid),
      Text(extent={{-92,50},{92,-50}}, textString="N14\nPITCH COMMAND LAG", fontSize=6)}));
  end N14PitchCommandLag;

  model N4ScheduledGain
    T.SignalInput u annotation(Placement(transformation(extent={{-110,-10},{-90,10}})));
    T.SignalInput impactPressurePsf
      annotation(Placement(transformation(extent={{-10,-110},{10,-90}})));
    T.ModeInput note1Active
      annotation(Placement(transformation(extent={{-10,90},{10,110}})));
    T.SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  equation
    y = u * n4Value(impactPressurePsf, note1Active > 0.5);
    annotation(Icon(graphics={
      Rectangle(extent={{-100,65},{100,-65}}, lineColor={0,0,0},
        fillColor={255,255,255}, fillPattern=FillPattern.Solid),
      Text(extent={{-92,40},{92,-40}}, textString="N4", fontSize=8)}));
  end N4ScheduledGain;

  model N7ScheduledGain
    T.SignalInput u annotation(Placement(transformation(extent={{-110,-10},{-90,10}})));
    T.SignalInput impactPressurePsf
      annotation(Placement(transformation(extent={{-10,-110},{10,-90}})));
    T.ModeInput note1Active
      annotation(Placement(transformation(extent={{-10,90},{10,110}})));
    T.SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  equation
    y = u * n7Value(impactPressurePsf, note1Active > 0.5);
    annotation(Icon(graphics={
      Rectangle(extent={{-100,65},{100,-65}}, lineColor={0,0,0},
        fillColor={255,255,255}, fillPattern=FillPattern.Solid),
      Text(extent={{-92,40},{92,-40}}, textString="N7", fontSize=8)}));
  end N7ScheduledGain;
end LongitudinalGains;

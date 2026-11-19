within ControlLawDiagrams;
package Transcription
  "Topology symbols for source-faithful control-law transcription"

  connector SignalInput = input Real "Input crossing the Figure 3.1 diagram boundary"
    annotation(Icon(graphics={
      Polygon(points={{-100,100},{100,0},{-100,-100},{-100,100}}, lineColor={0,0,127}, fillColor={0,0,127}, fillPattern=FillPattern.Solid)}));

  connector SignalOutput = output Real "Output crossing the Figure 3.1 diagram boundary"
    annotation(Icon(graphics={
      Polygon(points={{-100,100},{100,0},{-100,-100},{-100,100}}, lineColor={0,0,127}, fillColor={255,255,255}, fillPattern=FillPattern.Solid)}));

  connector ModeInput = input Real(min=0.0, max=1.0)
    "Discrete 0/1 mode state entering the Figure 3.1 law";
  connector ModeOutput = output Real(min=0.0, max=1.0)
    "Discrete 0/1 mode state delivered inside the Figure 3.1 law";

  model UnknownBlock
    parameter String label = "UNRESOLVED";
    SignalInput u annotation(Placement(transformation(extent={{-110,-10},{-90,10}})));
    SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  initial equation
    assert(false, "Unresolved source block: " + label);
  equation
    y = u;
    annotation(Icon(graphics={
      Rectangle(extent={{-100,65},{100,-65}}, lineColor={180,0,0}, fillColor={255,235,235}, fillPattern=FillPattern.Solid),
      Text(extent={{-92,40},{92,-40}}, textColor={180,0,0}, textString="%label", fontSize=6)}));
  end UnknownBlock;

  model UnresolvedGain
    "Known gain function whose source value is unreadable"
    parameter String label = "GAIN";
    parameter String unknownId = "[UNRESOLVED]";
    SignalInput u annotation(Placement(transformation(extent={{-110,-10},{-90,10}})));
    SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  initial equation
    assert(false, "Unresolved source gain " + unknownId + ": " + label);
  equation
    y = u;
    annotation(Icon(graphics={
      Rectangle(extent={{-100,65},{100,-65}}, lineColor={0,0,0},
        fillColor={255,255,255}, fillPattern=FillPattern.Solid),
      Text(extent={{-92,52},{92,2}}, textString="%label", fontSize=6),
      Text(extent={{-92,-5},{92,-50}}, textColor={180,0,0},
        textString="%unknownId", fontSize=6)}));
  end UnresolvedGain;

  model UnknownSource
    parameter String label = "UNRESOLVED";
    SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  initial equation
    assert(false, "Unresolved source: " + label);
  equation
    y = 0.0;
    annotation(Icon(graphics={
      Rectangle(extent={{-100,65},{100,-65}}, lineColor={180,0,0}, fillColor={255,235,235}, fillPattern=FillPattern.Solid),
      Text(extent={{-92,40},{92,-40}}, textColor={180,0,0}, textString="%label", fontSize=6)}));
  end UnknownSource;

  model Gain
    parameter Real k = 1.0;
    parameter String label = "GAIN";
    SignalInput u annotation(Placement(transformation(extent={{-110,-10},{-90,10}})));
    SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  equation
    y = k * u;
    annotation(Icon(graphics={
      Rectangle(extent={{-100,65},{100,-65}}, lineColor={0,0,0}, fillColor={255,255,255}, fillPattern=FillPattern.Solid),
      Text(extent={{-92,40},{92,-40}}, textString="%label", fontSize=6)}));
  end Gain;

  model AbsoluteValue
    parameter String label = "ABS VAL";
    SignalInput u annotation(Placement(transformation(extent={{-110,-10},{-90,10}})));
    SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  equation
    y = abs(u);
    annotation(Icon(graphics={
      Rectangle(extent={{-100,65},{100,-65}}, lineColor={0,0,0}, fillColor={255,255,255}, fillPattern=FillPattern.Solid),
      Text(extent={{-92,40},{92,-40}}, textString="%label", fontSize=6)}));
  end AbsoluteValue;

  model FirstOrder
    parameter Real pole(min=0.0) = 1.0;
    parameter String label = "a / (s + a)";
    Real state(start=0.0, fixed=true);
    SignalInput u annotation(Placement(transformation(extent={{-110,-10},{-90,10}})));
    SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  equation
    der(state) = pole * (u - state);
    y = state;
    annotation(Icon(graphics={
      Rectangle(extent={{-100,65},{100,-65}}, lineColor={0,0,0}, fillColor={255,255,255}, fillPattern=FillPattern.Solid),
      Text(extent={{-92,40},{92,-40}}, textString="%label", fontSize=6)}));
  end FirstOrder;

  model AnalogDemodulator
    "Cascaded analog-demodulator lag shown in Figure 3.1"
    Real firstStage(start=0.0, fixed=true);
    Real secondStage(start=0.0, fixed=true);
    SignalInput u annotation(Placement(transformation(extent={{-110,-10},{-90,10}})));
    SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  equation
    der(firstStage) = 293.0 * (u - firstStage);
    der(secondStage) = 112.0 * (firstStage - secondStage);
    y = secondStage;
    annotation(Icon(graphics={
      Rectangle(extent={{-100,65},{100,-55}},lineColor={0,0,0},fillColor={255,255,255},fillPattern=FillPattern.Solid),
      Text(extent={{-92,55},{-20,20}},textString="293",fontSize=7),
      Line(points={{-88,15},{-22,15}},color={0,0,0},thickness=1.0),
      Text(extent={{-92,10},{-20,-28}},textString="s + 293",fontSize=7),
      Text(extent={{-18,35},{18,-15}},textString="X",fontSize=8),
      Text(extent={{20,55},{92,20}},textString="112",fontSize=7),
      Line(points={{22,15},{88,15}},color={0,0,0},thickness=1.0),
      Text(extent={{20,10},{92,-28}},textString="s + 112",fontSize=7),
      Text(extent={{-100,-62},{100,-100}},textString="Analog Demod",fontSize=6)}));
  end AnalogDemodulator;

  model SoftwareBreakout
    "Symmetric 1.75 lbf software breakout shown in Figure 3.1"
    parameter Real threshold = 1.75;
    SignalInput u annotation(Placement(transformation(extent={{-110,-10},{-90,10}})));
    SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  equation
    y = if u > threshold then u - threshold else if u < -threshold then u + threshold else 0.0;
    annotation(Icon(graphics={
      Rectangle(extent={{-100,65},{100,-55}},lineColor={0,0,0},fillColor={255,255,255},fillPattern=FillPattern.Solid),
      Line(points={{-82,0},{82,0}},color={0,0,0}),
      Line(points={{0,-45},{0,48}},color={0,0,0}),
      Line(points={{-82,-42},{-25,0}},color={0,0,0},thickness=1.5),
      Line(points={{25,0},{82,42}},color={0,0,0},thickness=1.5),
      Text(extent={{-90,58},{-25,25}},textString="-1.75",fontSize=6),
      Text(extent={{25,-25},{90,-52}},textString="1.75",fontSize=6),
      Text(extent={{-100,-62},{100,-100}},textString="Software Breakout",fontSize=6)}));
  end SoftwareBreakout;

  model Integrator
    parameter Real k = 1.0;
    parameter String label = "k / s";
    Real state(start=0.0, fixed=true);
    SignalInput u annotation(Placement(transformation(extent={{-110,-10},{-90,10}})));
    SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  equation
    der(state) = k * u;
    y = state;
    annotation(Icon(graphics={
      Rectangle(extent={{-100,65},{100,-65}}, lineColor={0,0,0}, fillColor={255,255,255}, fillPattern=FillPattern.Solid),
      Text(extent={{-92,40},{92,-40}}, textString="%label", fontSize=6)}));
  end Integrator;

  model Constant
    parameter Real k = 0.0;
    parameter String label = "0.0";
    SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  equation
    y = k;
    annotation(Icon(graphics={
      Rectangle(extent={{-100,65},{100,-65}}, lineColor={0,0,0}, fillColor={255,255,255}, fillPattern=FillPattern.Solid),
      Text(extent={{-92,40},{92,-40}}, textString="%label", fontSize=6)}));
  end Constant;

  model DataValue
    "Unframed data value printed beside a signal path"
    parameter Real k = 0.0;
    parameter String label = "0.0";
    SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  equation
    y = k;
    annotation(Icon(graphics={
      Text(extent={{-100,45},{100,-45}},textString="%label",fontSize=7)}));
  end DataValue;

  model RepeatedNetRight
    "Right-facing repeated label for one shared source signal"
    parameter String label = "NET";
    SignalInput source annotation(Placement(visible=false,
      transformation(extent={{-10,90},{10,110}})));
    SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  equation
    y = source;
    annotation(Icon(graphics={
      Text(extent={{-90,48},{75,-48}}, textString="%label", fontSize=7)}));
  end RepeatedNetRight;

  model RepeatedNetLeft
    "Left-facing repeated label for one shared source signal"
    parameter String label = "NET";
    SignalInput source annotation(Placement(visible=false,
      transformation(extent={{-10,90},{10,110}})));
    SignalOutput y annotation(Placement(transformation(
      origin={-100,0}, extent={{-10,-10},{10,10}}, rotation=180)));
  equation
    y = source;
    annotation(Icon(graphics={
      Text(extent={{-75,48},{90,-48}}, textString="%label", fontSize=7)}));
  end RepeatedNetLeft;

  model RepeatedNetUp
    "Up-facing repeated label for one shared source signal"
    parameter String label = "NET";
    SignalInput source annotation(Placement(visible=false,
      transformation(extent={{-10,-110},{10,-90}})));
    SignalOutput y annotation(Placement(transformation(
      origin={0,100}, extent={{-10,-10},{10,10}}, rotation=90)));
  equation
    y = source;
    annotation(Icon(graphics={
      Text(extent={{-80,50},{80,-50}}, textString="%label", fontSize=7)}));
  end RepeatedNetUp;

  model Sum2
    parameter Real k1 = 1.0;
    parameter Real k2 = 1.0;
    parameter String label = "SUM";
    final parameter String sign1 = if k1 >= 0.0 then "+" else "-";
    final parameter String sign2 = if k2 >= 0.0 then "+" else "-";
    SignalInput u1 annotation(Placement(transformation(extent={{-110,-10},{-90,10}})));
    SignalInput u2 annotation(Placement(transformation(extent={{-10,-110},{10,-90}})));
    SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  equation
    y = k1 * u1 + k2 * u2;
    annotation(Icon(graphics={
      Ellipse(extent={{-75,75},{75,-75}}, lineColor={0,0,0}, fillColor={255,255,255}, fillPattern=FillPattern.Solid),
      Line(points={{-100,0},{-75,0}}, color={0,0,0}),
      Line(points={{75,0},{100,0}}, color={0,0,0}),
      Line(points={{0,-75},{0,-100}}, color={0,0,0}),
      Line(points={{-50,50},{50,-50}}, color={0,0,0}), Line(points={{-50,-50},{50,50}}, color={0,0,0}),
      Text(extent={{-72,25},{-22,-25}},textString="%sign1",fontSize=7),
      Text(extent={{-25,-72},{25,-22}},textString="%sign2",fontSize=7),
      Text(extent={{-100,145},{100,82}}, textString="%label", fontSize=6)}));
  end Sum2;

  model Sum2Top
    "Two-input summing node with the secondary input entering from above"
    parameter Real k1 = 1.0;
    parameter Real k2 = 1.0;
    parameter String label = "SUM";
    final parameter String sign1 = if k1 >= 0.0 then "+" else "-";
    final parameter String sign2 = if k2 >= 0.0 then "+" else "-";
    SignalInput u1 annotation(Placement(transformation(extent={{-110,-10},{-90,10}})));
    SignalInput u2 annotation(Placement(transformation(extent={{-10,90},{10,110}})));
    SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  equation
    y = k1 * u1 + k2 * u2;
    annotation(Icon(graphics={
      Ellipse(extent={{-75,75},{75,-75}}, lineColor={0,0,0}, fillColor={255,255,255}, fillPattern=FillPattern.Solid),
      Line(points={{-100,0},{-75,0}}, color={0,0,0}),
      Line(points={{75,0},{100,0}}, color={0,0,0}),
      Line(points={{0,75},{0,100}}, color={0,0,0}),
      Line(points={{-50,50},{50,-50}}, color={0,0,0}),
      Line(points={{-50,-50},{50,50}}, color={0,0,0}),
      Text(extent={{-72,25},{-22,-25}},textString="%sign1",fontSize=7),
      Text(extent={{-25,72},{25,22}},textString="%sign2",fontSize=7),
      Text(extent={{-100,145},{100,82}}, textString="%label", fontSize=6)}));
  end Sum2Top;

  model SumVertical2
    "Two-input summing node with inputs entering from above and below"
    parameter Real kTop = 1.0;
    parameter Real kBottom = 1.0;
    parameter String label = "SUM";
    final parameter String topSign = if kTop >= 0.0 then "+" else "-";
    final parameter String bottomSign = if kBottom >= 0.0 then "+" else "-";
    SignalInput top annotation(Placement(transformation(extent={{-10,90},{10,110}})));
    SignalInput bottom annotation(Placement(transformation(extent={{-10,-110},{10,-90}})));
    SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  equation
    y = kTop * top + kBottom * bottom;
    annotation(Icon(graphics={
      Ellipse(extent={{-75,75},{75,-75}},lineColor={0,0,0},fillColor={255,255,255},fillPattern=FillPattern.Solid),
      Line(points={{0,100},{0,75}},color={0,0,0}),
      Line(points={{0,-75},{0,-100}},color={0,0,0}),
      Line(points={{75,0},{100,0}},color={0,0,0}),
      Line(points={{-50,50},{50,-50}},color={0,0,0}),
      Line(points={{-50,-50},{50,50}},color={0,0,0}),
      Text(extent={{-25,72},{25,22}},textString="%topSign",fontSize=7),
      Text(extent={{-25,-72},{25,-22}},textString="%bottomSign",fontSize=7),
      Text(extent={{-100,145},{100,82}},textString="%label",fontSize=6)}));
  end SumVertical2;

  model Sum1
    parameter String label = "SUM";
    SignalInput u annotation(Placement(transformation(extent={{-110,-10},{-90,10}})));
    SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  equation
    y = u;
    annotation(Icon(graphics={
      Ellipse(extent={{-75,75},{75,-75}}, lineColor={0,0,0}, fillColor={255,255,255}, fillPattern=FillPattern.Solid),
      Line(points={{-100,0},{-75,0}}, color={0,0,0}),
      Line(points={{75,0},{100,0}}, color={0,0,0}),
      Line(points={{-50,50},{50,-50}}, color={0,0,0}), Line(points={{-50,-50},{50,50}}, color={0,0,0}),
      Text(extent={{-100,145},{100,82}}, textString="%label", fontSize=6)}));
  end Sum1;

  model Sum1BottomBranch
    "Single-input circular node with distinct right and bottom outputs"
    parameter String label = "SUM";
    SignalInput u annotation(Placement(transformation(extent={{-110,-10},{-90,10}})));
    SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
    SignalOutput yBottom
      annotation(Placement(transformation(extent={{-10,-110},{10,-90}})));
  equation
    y = u;
    yBottom = u;
    annotation(Icon(graphics={
      Ellipse(extent={{-75,75},{75,-75}}, lineColor={0,0,0},
        fillColor={255,255,255}, fillPattern=FillPattern.Solid),
      Line(points={{-100,0},{-75,0}}, color={0,0,0}),
      Line(points={{75,0},{100,0}}, color={0,0,0}),
      Line(points={{0,-75},{0,-100}}, color={0,0,0}),
      Line(points={{-50,50},{50,-50}}, color={0,0,0}),
      Line(points={{-50,-50},{50,50}}, color={0,0,0}),
      Text(extent={{-100,145},{100,82}}, textString="%label", fontSize=6)}));
  end Sum1BottomBranch;

  model DifferenceLeftRightToBottom
    "Opposed horizontal inputs with the result leaving through the bottom"
    parameter String label = "SUM";
    SignalInput left annotation(Placement(transformation(extent={{-110,-10},{-90,10}})));
    SignalInput right annotation(Placement(transformation(
      origin={100,0}, extent={{-10,-10},{10,10}}, rotation=180)));
    SignalOutput bottom
      annotation(Placement(transformation(
        origin={0,-100}, extent={{-10,-10},{10,10}}, rotation=-90)));
  equation
    bottom = left - right;
    annotation(Icon(graphics={
      Ellipse(extent={{-75,75},{75,-75}}, lineColor={0,0,0},
        fillColor={255,255,255}, fillPattern=FillPattern.Solid),
      Line(points={{-100,0},{-75,0}}, color={0,0,0}),
      Line(points={{75,0},{100,0}}, color={0,0,0}),
      Line(points={{0,-75},{0,-100}}, color={0,0,0}),
      Line(points={{-50,50},{50,-50}}, color={0,0,0}),
      Line(points={{-50,-50},{50,50}}, color={0,0,0}),
      Text(extent={{-72,25},{-22,-25}}, textString="+", fontSize=7),
      Text(extent={{22,25},{72,-25}}, textString="-", fontSize=7),
      Text(extent={{-100,145},{100,82}}, textString="%label", fontSize=6)}));
  end DifferenceLeftRightToBottom;

  model Sum3
    parameter Real k1 = 1.0;
    parameter Real k2 = 1.0;
    parameter Real k3 = 1.0;
    parameter String label = "SUM";
    final parameter String sign1 = if k1 >= 0.0 then "+" else "-";
    final parameter String sign2 = if k2 >= 0.0 then "+" else "-";
    final parameter String sign3 = if k3 >= 0.0 then "+" else "-";
    SignalInput u1 annotation(Placement(transformation(extent={{-110,-10},{-90,10}})));
    SignalInput u2 annotation(Placement(transformation(extent={{-10,90},{10,110}})));
    SignalInput u3 annotation(Placement(transformation(extent={{-10,-110},{10,-90}})));
    SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  equation
    y = k1 * u1 + k2 * u2 + k3 * u3;
    annotation(Icon(graphics={
      Ellipse(extent={{-75,75},{75,-75}}, lineColor={0,0,0}, fillColor={255,255,255}, fillPattern=FillPattern.Solid),
      Line(points={{-100,0},{-75,0}}, color={0,0,0}),
      Line(points={{75,0},{100,0}}, color={0,0,0}),
      Line(points={{0,75},{0,100}}, color={0,0,0}),
      Line(points={{0,-75},{0,-100}}, color={0,0,0}),
      Line(points={{-50,50},{50,-50}}, color={0,0,0}), Line(points={{-50,-50},{50,50}}, color={0,0,0}),
      Text(extent={{-72,25},{-22,-25}},textString="%sign1",fontSize=7),
      Text(extent={{-25,72},{25,22}},textString="%sign2",fontSize=7),
      Text(extent={{-25,-72},{25,-22}},textString="%sign3",fontSize=7),
      Text(extent={{-100,145},{100,82}}, textString="%label", fontSize=6)}));
  end Sum3;

  model Monitor
    parameter String label = "S";
    SignalInput u annotation(Placement(transformation(extent={{-110,-10},{-90,10}})));
    SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
    SignalOutput tap annotation(Placement(transformation(extent={{-10,90},{10,110}})));
  equation
    y = u;
    tap = u;
    annotation(Icon(graphics={
      Ellipse(extent={{-75,75},{75,-75}}, lineColor={0,0,0}, fillColor={255,255,255}, fillPattern=FillPattern.Solid),
      Line(points={{-100,0},{-75,0}}, color={0,0,0}),
      Line(points={{75,0},{100,0}}, color={0,0,0}),
      Line(points={{0,75},{0,100}}, color={0,0,0}),
      Text(extent={{-55,40},{55,-40}}, textString="%label", fontSize=14)}));
  end Monitor;

  model Product2
    parameter String label = "X";
    SignalInput u1 annotation(Placement(transformation(extent={{-110,-10},{-90,10}})));
    SignalInput u2 annotation(Placement(transformation(extent={{-10,-110},{10,-90}})));
    SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  equation
    y = u1 * u2;
    annotation(Icon(graphics={
      Rectangle(extent={{-70,70},{70,-70}}, lineColor={0,0,0}, fillColor={255,255,255}, fillPattern=FillPattern.Solid),
      Line(points={{-100,0},{-70,0}}, color={0,0,0}),
      Line(points={{70,0},{100,0}}, color={0,0,0}),
      Line(points={{0,-70},{0,-100}}, color={0,0,0}),
      Line(points={{-48,48},{48,-48}}, color={0,0,0}), Line(points={{-48,-48},{48,48}}, color={0,0,0}),
      Text(extent={{-100,145},{100,82}}, textString="%label", fontSize=6)}));
  end Product2;

  model ProductRightBottomToLeft
    "Multiplier with right and bottom inputs and a left output"
    parameter String label = "X";
    SignalInput right annotation(Placement(transformation(
      origin={100,0}, extent={{-10,-10},{10,10}}, rotation=180)));
    SignalInput bottom annotation(Placement(transformation(
      origin={0,-100}, extent={{-10,-10},{10,10}}, rotation=90)));
    SignalOutput left annotation(Placement(transformation(
      origin={-100,0}, extent={{-10,-10},{10,10}}, rotation=180)));
  equation
    left = right * bottom;
    annotation(Icon(graphics={
      Rectangle(extent={{-70,70},{70,-70}}, lineColor={0,0,0},
        fillColor={255,255,255}, fillPattern=FillPattern.Solid),
      Line(points={{-100,0},{-70,0}}, color={0,0,0}),
      Line(points={{70,0},{100,0}}, color={0,0,0}),
      Line(points={{0,-70},{0,-100}}, color={0,0,0}),
      Line(points={{-48,48},{48,-48}}, color={0,0,0}),
      Line(points={{-48,-48},{48,48}}, color={0,0,0}),
      Text(extent={{-100,145},{100,82}}, textString="%label", fontSize=6)}));
  end ProductRightBottomToLeft;

  model Product2Top
    "Two-input multiplier with the secondary input entering from above"
    parameter String label = "X";
    SignalInput u1 annotation(Placement(transformation(extent={{-110,-10},{-90,10}})));
    SignalInput u2 annotation(Placement(transformation(extent={{-10,90},{10,110}})));
    SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  equation
    y = u1 * u2;
    annotation(Icon(graphics={
      Rectangle(extent={{-70,70},{70,-70}}, lineColor={0,0,0}, fillColor={255,255,255}, fillPattern=FillPattern.Solid),
      Line(points={{-100,0},{-70,0}}, color={0,0,0}),
      Line(points={{70,0},{100,0}}, color={0,0,0}),
      Line(points={{0,70},{0,100}}, color={0,0,0}),
      Line(points={{-48,48},{48,-48}}, color={0,0,0}),
      Line(points={{-48,-48},{48,48}}, color={0,0,0}),
      Text(extent={{-100,145},{100,82}}, textString="%label", fontSize=6)}));
  end Product2Top;

  model Selector2
    parameter String label = "MODE";
    SignalInput upper annotation(Placement(transformation(extent={{-110,45},{-90,65}})));
    SignalInput lower annotation(Placement(transformation(extent={{-110,-65},{-90,-45}})));
    ModeInput control annotation(Placement(transformation(extent={{-10,-110},{10,-90}})));
    SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  equation
    // The source diagrams show the unactivated arm on input 1 (upper).
    // Activating the labeled mode transfers the arm to input 2 (lower).
    y = if control > 0.5 then lower else upper;
    annotation(Icon(graphics={
      Line(points={{-100,55},{-45,55}}, color={0,0,0}),
      Line(points={{-100,-55},{-45,-55}}, color={0,0,0}),
      Ellipse(extent={{-43,63},{-27,47}},lineColor={0,0,0},fillColor={0,0,0},fillPattern=FillPattern.Solid),
      Ellipse(extent={{-43,-47},{-27,-63}},lineColor={0,0,0},fillColor={0,0,0},fillPattern=FillPattern.Solid),
      Line(points={{55,0},{-27,47}}, color={0,0,0}, thickness=1.5),
      Ellipse(extent={{45,10},{65,-10}},lineColor={0,0,0},fillColor={0,0,0},fillPattern=FillPattern.Solid),
      Line(points={{55,0},{100,0}}, color={0,0,0}),
      Line(points={{0,-100},{0,-65}}, color={120,0,120}, pattern=LinePattern.Dash),
      Text(extent={{-82,82},{-52,58}},textString="1",fontSize=7),
      Text(extent={{-82,-58},{-52,-82}},textString="2",fontSize=7),
      Text(extent={{-100,145},{100,82}}, textString="%label", fontSize=6)}));
  end Selector2;

  model Limiter
    parameter Real lower = -1.0;
    parameter Real upper = 1.0;
    parameter String label = "LIMITER";
    SignalInput u annotation(Placement(transformation(extent={{-110,-10},{-90,10}})));
    SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  equation
    y = min(max(u, lower), upper);
    annotation(Icon(graphics={
      Rectangle(extent={{-100,65},{100,-65}}, lineColor={0,0,0}, fillColor={255,255,255}, fillPattern=FillPattern.Solid),
      Line(points={{-80,-42},{-45,-42},{45,42},{80,42}}, color={0,0,0}),
      Text(extent={{-92,40},{92,-40}}, textString="%label", fontSize=6)}));
  end Limiter;

  model LimiterWithUnknownBounds
    "Source-confirmed saturation limiter whose two numerical bounds remain unreadable"
    parameter String upperUnknownId = "[U-046]";
    parameter String lowerUnknownId = "[U-047]";
    SignalInput u annotation(Placement(transformation(extent={{-110,-10},{-90,10}})));
    SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  initial equation
    assert(false,
      "Unresolved limiter bounds: " + upperUnknownId + " and " + lowerUnknownId);
  equation
    y = u;
    annotation(Icon(graphics={
      Rectangle(extent={{-100,65},{100,-65}},lineColor={0,0,0},
        fillColor={255,255,255},fillPattern=FillPattern.Solid),
      Line(points={{-80,-42},{-45,-42},{45,42},{80,42}},color={0,0,0}),
      Text(extent={{25,62},{95,34}},textString="%upperUnknownId",
        textColor={180,0,0},fontSize=6),
      Text(extent={{-95,-34},{-25,-62}},textString="%lowerUnknownId",
        textColor={180,0,0},fontSize=6),
      Text(extent={{-100,105},{100,72}},textString="LIMITER",fontSize=6)}));
  end LimiterWithUnknownBounds;

  model VariableLimiter
    parameter String label = "LIMITER";
    SignalInput u annotation(Placement(transformation(extent={{-110,-10},{-90,10}})));
    SignalInput upperLimit annotation(Placement(transformation(extent={{-10,90},{10,110}})));
    SignalInput lowerLimit annotation(Placement(transformation(extent={{-10,-110},{10,-90}})));
    SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  equation
    assert(upperLimit >= lowerLimit, "Variable limiter bounds are reversed");
    y = min(max(u, lowerLimit), upperLimit);
    annotation(Icon(graphics={
      Rectangle(extent={{-100,65},{100,-65}}, lineColor={0,0,0}, fillColor={255,255,255}, fillPattern=FillPattern.Solid),
      Line(points={{-80,-42},{-45,-42},{45,42},{80,42}}, color={0,0,0}),
      Text(extent={{-92,40},{92,-40}}, textString="%label", fontSize=6)}));
  end VariableLimiter;

  model PositiveValue
    parameter String label = "POS VAL";
    SignalInput u annotation(Placement(transformation(extent={{-110,-10},{-90,10}})));
    SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  equation
    y = max(0.0, u);
    annotation(Icon(graphics={
      Rectangle(extent={{-100,65},{100,-65}}, lineColor={0,0,0}, fillColor={255,255,255}, fillPattern=FillPattern.Solid),
      Line(points={{-75,-40},{-15,-40},{65,40}}, color={0,0,0}),
      Text(extent={{-92,40},{92,-40}}, textString="%label", fontSize=6)}));
  end PositiveValue;

  model Max2
    parameter String label = "MAX";
    SignalInput u1 annotation(Placement(transformation(extent={{-110,35},{-90,55}})));
    SignalInput u2 annotation(Placement(transformation(extent={{-110,-55},{-90,-35}})));
    SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  equation
    y = max(u1, u2);
    annotation(Icon(graphics={
      Rectangle(extent={{-100,65},{100,-65}}, lineColor={0,0,0}, fillColor={255,255,255}, fillPattern=FillPattern.Solid),
      Text(extent={{-92,40},{92,-40}}, textString="%label", fontSize=6)}));
  end Max2;

  model Junction
    "Explicit signal branch point"
    SignalInput pin annotation(Placement(transformation(extent={{-10,-10},{10,10}})));
    annotation(Icon(graphics={
      Ellipse(extent={{-22,22},{22,-22}}, lineColor={0,0,0}, fillColor={0,0,0}, fillPattern=FillPattern.Solid)}));
  end Junction;

  model HorizontalCrossover
    "Visual bridge showing that a horizontal signal crosses without connecting"
    annotation(Icon(graphics={
      Line(points={{-100,0},{-45,0},{0,42},{45,0},{100,0}},color={255,255,255},thickness=5,smooth=Smooth.Bezier),
      Line(points={{-100,0},{-45,0},{0,42},{45,0},{100,0}},color={0,0,0},thickness=1.5,smooth=Smooth.Bezier)}));
  end HorizontalCrossover;

  model Terminal
    parameter String label = "PORT";
    SignalInput pin annotation(Placement(transformation(extent={{-10,-10},{10,10}})));
    annotation(Icon(graphics={
      Polygon(points={{-100,0},{-55,55},{55,55},{100,0},{55,-55},{-55,-55},{-100,0}}, lineColor={0,0,0}, fillColor={255,255,255}, fillPattern=FillPattern.Solid),
      Text(extent={{-80,36},{80,-36}}, textString="%label", fontSize=6)}));
  end Terminal;

  model ModeControl
    "Unframed mode label feeding a selector control input"
    parameter String label = "MODE";
    ModeInput u annotation(Placement(visible=false, transformation(extent={{-10,90},{10,110}})));
    ModeOutput pin annotation(Placement(transformation(extent={{-10,-10},{10,10}})));
  equation
    pin = u;
    annotation(Icon(graphics={
      Text(extent={{-100,-10},{100,-70}},textString="%label",fontSize=7)}));
  end ModeControl;

  annotation(Documentation(info="<html><p>These symbols preserve source topology. Unknown blocks deliberately fail initialization so a transcription cannot be mistaken for a validated executable control law.</p></html>"));
end Transcription;

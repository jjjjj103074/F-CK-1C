within ControlLawDiagrams;
package Dynamics
  "Executable dynamic blocks used by the Figure 3.1 transcription"
  import T = ControlLawDiagrams.Transcription;

  model SecondOrderLowPass
    "b0/(s^2 + a1*s + a0)"
    parameter Real numerator = 1.0;
    parameter Real damping = 1.0;
    parameter Real naturalSquared = 1.0;
    parameter String label = "SECOND ORDER";
    Real position(start=0.0, fixed=true);
    Real rate(start=0.0, fixed=true);
    T.SignalInput u annotation(Placement(transformation(extent={{-110,-10},{-90,10}})));
    T.SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  equation
    assert(damping > 0.0, "Second-order damping coefficient must be positive");
    assert(naturalSquared > 0.0, "Second-order natural-frequency square must be positive");
    der(position) = rate;
    der(rate) = numerator * u - damping * rate - naturalSquared * position;
    y = position;
    annotation(Icon(graphics={
      Rectangle(extent={{-100,65},{100,-65}},lineColor={0,0,0},
        fillColor={255,255,255},fillPattern=FillPattern.Solid),
      Text(extent={{-92,42},{92,-42}},textString="%label",fontSize=6)}));
  end SecondOrderLowPass;

  model Washout
    "s/(s+a)"
    parameter Real pole(min=Modelica.Constants.small) = 1.0;
    parameter String label = "s / (s + a)";
    Real lowPassState(start=0.0, fixed=true);
    T.SignalInput u annotation(Placement(transformation(extent={{-110,-10},{-90,10}})));
    T.SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  equation
    der(lowPassState) = pole * (u - lowPassState);
    y = u - lowPassState;
    annotation(Icon(graphics={
      Rectangle(extent={{-100,65},{100,-65}},lineColor={0,0,0},
        fillColor={255,255,255},fillPattern=FillPattern.Solid),
      Text(extent={{-92,42},{92,-42}},textString="%label",fontSize=6)}));
  end Washout;

  model LeadLag
    "(lead*s + pole)/(s + pole)"
    parameter Real lead = 1.0;
    parameter Real pole(min=Modelica.Constants.small) = 1.0;
    parameter String label = "LEAD / LAG";
    Real lowPassState(start=0.0, fixed=true);
    T.SignalInput u annotation(Placement(transformation(extent={{-110,-10},{-90,10}})));
    T.SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  equation
    der(lowPassState) = pole * (u - lowPassState);
    y = lead * u + (1.0 - lead) * lowPassState;
    annotation(Icon(graphics={
      Rectangle(extent={{-100,65},{100,-65}},lineColor={0,0,0},
        fillColor={255,255,255},fillPattern=FillPattern.Solid),
      Text(extent={{-92,42},{92,-42}},textString="%label",fontSize=6)}));
  end LeadLag;

  model VariableLeadLag
    "(lead*s + pole)/(s + pole) with a scheduled pole"
    parameter Real lead = 1.0;
    parameter String label = "SCHEDULED LEAD / LAG";
    Real lowPassState(start=0.0, fixed=true);
    T.SignalInput u annotation(Placement(transformation(extent={{-110,-10},{-90,10}})));
    T.SignalInput pole annotation(Placement(transformation(extent={{-10,-110},{10,-90}})));
    T.SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  equation
    assert(pole > 0.0, "Scheduled lead-lag pole must be positive");
    der(lowPassState) = pole * (u - lowPassState);
    y = lead * u + (1.0 - lead) * lowPassState;
    annotation(Icon(graphics={
      Rectangle(extent={{-100,65},{100,-65}},lineColor={0,0,0},
        fillColor={255,255,255},fillPattern=FillPattern.Solid),
      Text(extent={{-92,42},{92,-42}},textString="%label",fontSize=6)}));
  end VariableLeadLag;

  model Deadband
    "Symmetric deadband; width is the full zero-output interval"
    parameter Real width(min=0.0) = 0.0;
    parameter Real gain = 1.0;
    parameter String label = "DEADBAND";
    final parameter Real halfWidth = width / 2.0;
    T.SignalInput u annotation(Placement(transformation(extent={{-110,-10},{-90,10}})));
    T.SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  equation
    y = if u > halfWidth then gain * (u - halfWidth)
      else if u < -halfWidth then gain * (u + halfWidth) else 0.0;
    annotation(Icon(graphics={
      Rectangle(extent={{-100,65},{100,-65}},lineColor={0,0,0},
        fillColor={255,255,255},fillPattern=FillPattern.Solid),
      Line(points={{-82,-45},{-20,0},{20,0},{82,45}},color={0,0,0},thickness=1.5),
      Text(extent={{-92,42},{92,-42}},textString="%label",fontSize=6)}));
  end Deadband;

  model LimitedIntegrator
    "Integrator whose state stops at the stated physical output limits"
    parameter Real k = 1.0;
    parameter Real lower = -1.0;
    parameter Real upper = 1.0;
    parameter String label = "LIMITED k / s";
    Real state(start=0.0, fixed=true);
    T.SignalInput u annotation(Placement(transformation(extent={{-110,-10},{-90,10}})));
    T.SignalOutput y annotation(Placement(transformation(extent={{90,-10},{110,10}})));
  equation
    assert(upper > lower, "Limited-integrator bounds are reversed");
    der(state) = if state >= upper and u > 0.0 then 0.0
      else if state <= lower and u < 0.0 then 0.0 else k * u;
    y = min(max(state, lower), upper);
    annotation(Icon(graphics={
      Rectangle(extent={{-100,65},{100,-65}},lineColor={0,0,0},
        fillColor={255,255,255},fillPattern=FillPattern.Solid),
      Text(extent={{-92,42},{92,-42}},textString="%label",fontSize=6)}));
  end LimitedIntegrator;
end Dynamics;

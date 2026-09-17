struct SmallSignalAircraft
{
	double altitude_m = kInitialAltitudeM;
	double vertical_speed_mps = 0.0;
	double magnetic_heading_rad = 0.0;
	double roll_rad = 0.0;
	double pitch_rad = 0.0;
	double roll_rate_rad_s = 0.0;
	double pitch_rate_rad_s = 0.0;
	double yaw_rate_rad_s = 0.0;
	double flight_path_angle_rad = 0.0;
	double angle_of_attack_rad = kTestAlphaRad;
	double normal_acceleration_g = kLevelNormalAccelerationG;
};

class ClosedLoopRig final
{
public:
	explicit ClosedLoopRig(double airspeed_mps = kTestAirspeedMps)
		: computer_(
			Core::Systems::fck1c_flight_control_computer_config(),
			Core::StartMode::HotAir,
			{ kNeutralControlNormalized, kNeutralControlNormalized }),
		  actuation_(
			Core::Systems::fck1c_flight_control_actuation_system_config()),
		  airspeed_mps_(airspeed_mps)
	{
	}

	void command(Core::CommandId id, double value = kPressedCommandValue)
	{
		computer_.handle_command({ id, value });
	}

	void tick()
	{
		const auto& demand = computer_.step(
			{ make_input(),
				{ kNeutralControlNormalized, kNeutralControlNormalized } });
		for (int index = 0; index < kActuatorStepsPerFccTick; ++index)
		{
			(void)actuation_.update(demand, kActuatorDtS);
		}
		update_aircraft();
	}

	const SmallSignalAircraft& aircraft() const { return aircraft_; }
	const Core::AutomaticFlightControlSnapshot& autopilot() const
	{
		return computer_.automatic_flight_control_snapshot();
	}
	const Core::FlightControlComputerSnapshot& diagnostics() const
	{
		return computer_.diagnostics();
	}
	bool actuator_saturated() const
	{
		return actuation_.state().any_saturated;
	}
	void offset_altitude(double delta_m)
	{
		aircraft_.altitude_m += delta_m;
	}
	void offset_pitch(double delta_rad)
	{
		aircraft_.pitch_rad += delta_rad;
	}
	void offset_roll(double delta_rad)
	{
		aircraft_.roll_rad += delta_rad;
	}
	void set_vertical_speed(double vertical_speed_mps)
	{
		aircraft_.vertical_speed_mps = vertical_speed_mps;
	}
	void set_pilot_pitch(double pitch_normalized)
	{
		if (!dynamic_alpha_enabled_)
		{
			aircraft_.flight_path_angle_rad = kTestAlphaRad +
				aircraft_.pitch_rad - aircraft_.angle_of_attack_rad;
		}
		pilot_pitch_normalized_ = pitch_normalized;
		dynamic_alpha_enabled_ = true;
	}

private:
	Core::Systems::RawFlightControlInput make_input() const
	{
		Core::Systems::RawFlightControlInput input;
		input.dt_s = kFccDtS;
		input.observation = {
			Common::feet(aircraft_.altitude_m), airspeed_mps_,
			Common::feet(aircraft_.vertical_speed_mps),
			kTestMach, kTestDynamicPressurePa,
			aircraft_.normal_acceleration_g,
			aircraft_.angle_of_attack_rad, 0.0,
			true, Common::deg(aircraft_.magnetic_heading_rad),
			aircraft_.roll_rad, aircraft_.pitch_rad,
			aircraft_.roll_rate_rad_s, aircraft_.pitch_rate_rad_s,
			aircraft_.yaw_rate_rad_s, true
		};
		input.pilot.pitch_axis_normalized = pilot_pitch_normalized_;
		input.actuator = actuation_.state();
		return input;
	}

	void update_aircraft()
	{
		const auto& surface = actuation_.state();
		const auto& config =
			Core::Systems::fck1c_flight_control_actuation_system_config();
		const double elevator = surface.symmetric_stabilator.position_rad /
			config.symmetric_stabilator.maximum_deflection_rad;
		const double aileron = surface.differential_flaperon.position_rad /
			config.differential_flaperon.maximum_deflection_rad;
		aircraft_.pitch_rate_rad_s +=
			(kPitchAccelerationGain * elevator -
				kPitchRateDamping * aircraft_.pitch_rate_rad_s) * kFccDtS;
		aircraft_.roll_rate_rad_s +=
			(kRollAccelerationGain * aileron -
				kRollRateDamping * aircraft_.roll_rate_rad_s) * kFccDtS;
		aircraft_.pitch_rad += aircraft_.pitch_rate_rad_s * kFccDtS;
		aircraft_.roll_rad += aircraft_.roll_rate_rad_s * kFccDtS;
		aircraft_.normal_acceleration_g =
			kLevelNormalAccelerationG + kNormalAccelerationGain * elevator;
		update_vertical_motion();
		aircraft_.yaw_rate_rad_s =
			kGravityMps2 * std::tan(aircraft_.roll_rad) / airspeed_mps_;
		aircraft_.magnetic_heading_rad = wrap_pi(
			aircraft_.magnetic_heading_rad +
				aircraft_.yaw_rate_rad_s * kFccDtS);
	}

	void update_vertical_motion()
	{
		if (dynamic_alpha_enabled_)
		{
			const double path_rate_rad_s = kGravityMps2 / airspeed_mps_ *
				(aircraft_.normal_acceleration_g -
					std::cos(aircraft_.flight_path_angle_rad));
			aircraft_.flight_path_angle_rad += path_rate_rad_s * kFccDtS;
			aircraft_.vertical_speed_mps = airspeed_mps_ *
				std::sin(aircraft_.flight_path_angle_rad);
			aircraft_.angle_of_attack_rad = wrap_pi(
				kTestAlphaRad + aircraft_.pitch_rad -
					aircraft_.flight_path_angle_rad);
		}
		else
		{
			const double vertical_acceleration_mps2 = kGravityMps2 *
				(aircraft_.normal_acceleration_g *
					std::cos(aircraft_.roll_rad) -
					kLevelNormalAccelerationG) -
				kVerticalSpeedDamping * aircraft_.vertical_speed_mps;
			aircraft_.vertical_speed_mps +=
				vertical_acceleration_mps2 * kFccDtS;
		}
		aircraft_.altitude_m += aircraft_.vertical_speed_mps * kFccDtS;
	}

	Core::Systems::FlightControlComputer computer_;
	Core::Systems::FlightControlActuationSystem actuation_;
	SmallSignalAircraft aircraft_;
	const double airspeed_mps_;
	double pilot_pitch_normalized_ = 0.0;
	bool dynamic_alpha_enabled_ = false;
};

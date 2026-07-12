#include "F-CK-1C_EFM_API.h"
#include "include/FM/wHumanCustomPhysicsAPI.h"

// EFMREF: DCS_CONTRACT - All exported prototypes in this block are DCS ABI callbacks.
// Keep names/signatures stable; move behavior behind these wrappers instead.
extern "C"
{
	// Force source callbacks. Body axis: X forward, Y up, Z right.
	F_CK_1C_EFM_API void ed_fm_add_local_force(double & x,double &y,double &z,double & pos_x,double & pos_y,double & pos_z);
	F_CK_1C_EFM_API void ed_fm_add_global_force(double & x,double &y,double &z,double & pos_x,double & pos_y,double & pos_z);

	// Optional component-form force callbacks.
	F_CK_1C_EFM_API bool ed_fm_add_local_force_component(double & x,double &y,double &z,double & pos_x,double & pos_y,double & pos_z);
	F_CK_1C_EFM_API bool ed_fm_add_global_force_component(double & x,double &y,double &z,double & pos_x,double & pos_y,double & pos_z);

	F_CK_1C_EFM_API void ed_fm_add_global_moment(double & x,double &y,double &z);
	F_CK_1C_EFM_API void ed_fm_add_local_moment(double & x,double &y,double &z);

	// Optional component-form moment callbacks.
	F_CK_1C_EFM_API bool ed_fm_add_local_moment_component(double & x,double &y,double &z);

	F_CK_1C_EFM_API bool ed_fm_add_global_moment_component(double & x,double &y,double &z);

	F_CK_1C_EFM_API void ed_fm_simulate(double dt);

	F_CK_1C_EFM_API void ed_fm_set_atmosphere(double h,//altitude above sea level
							 double t,//current atmosphere temperature
							 double a,//speed of sound
							 double ro,// atmosphere density
							 double p,// atmosphere pressure
							 double wind_vx,//components of velocity vector, including turbulence in world coordinate system
							 double wind_vy,//components of velocity vector, including turbulence in world coordinate system
							 double wind_vz //components of velocity vector, including turbulence in world coordinate system
							);
	// Called before simulation to provide mass and inertia state.
	F_CK_1C_EFM_API void ed_fm_set_current_mass_state (double mass,
										double center_of_mass_x,
										double center_of_mass_y,
										double center_of_mass_z,
										double moment_of_inertia_x,
										double moment_of_inertia_y,
										double moment_of_inertia_z
										);
	// Called before simulation to provide world-axis state.
	F_CK_1C_EFM_API void ed_fm_set_current_state (double ax,//linear acceleration component in world coordinate system
													   double ay,//linear acceleration component in world coordinate system
													   double az,//linear acceleration component in world coordinate system
													   double vx,//linear velocity component in world coordinate system
													   double vy,//linear velocity component in world coordinate system
													   double vz,//linear velocity component in world coordinate system
													   double px,//center of the body position in world coordinate system
													   double py,//center of the body position in world coordinate system
													   double pz,//center of the body position in world coordinate system
													   double omegadotx,//angular accelearation components in world coordinate system
													   double omegadoty,//angular accelearation components in world coordinate system
													   double omegadotz,//angular accelearation components in world coordinate system
													   double omegax,//angular velocity components in world coordinate system
													   double omegay,//angular velocity components in world coordinate system
													   double omegaz,//angular velocity components in world coordinate system
													   double quaternion_x,//orientation quaternion components in world coordinate system
													   double quaternion_y,//orientation quaternion components in world coordinate system
													   double quaternion_z,//orientation quaternion components in world coordinate system
													   double quaternion_w //orientation quaternion components in world coordinate system
													   );
	// Called before simulation to provide body-axis state.
	F_CK_1C_EFM_API void ed_fm_set_current_state_body_axis(double ax,//linear acceleration component in body coordinate system
															double ay,//linear acceleration component in body coordinate system
															double az,//linear acceleration component in body coordinate system
															double vx,//linear velocity component in body coordinate system
															double vy,//linear velocity component in body coordinate system
															double vz,//linear velocity component in body coordinate system
															double wind_vx,//wind linear velocity component in body coordinate system
															double wind_vy,//wind linear velocity component in body coordinate system
															double wind_vz,//wind linear velocity component in body coordinate system

															double omegadotx,//angular accelearation components in body coordinate system
															double omegadoty,//angular accelearation components in body coordinate system
															double omegadotz,//angular accelearation components in body coordinate system
															double omegax,//angular velocity components in body coordinate system
															double omegay,//angular velocity components in body coordinate system
															double omegaz,//angular velocity components in body coordinate system
															double yaw,  //radians
															double pitch,//radians
															double roll, //radians
															double common_angle_of_attack, //AoA radians
															double common_angle_of_slide   //AoS radians
															);
	// Input command callback.
	F_CK_1C_EFM_API void ed_fm_set_command (int command,
							 float value);
	// Mass-change callback. Returns false when no more deltas are pending.
	F_CK_1C_EFM_API bool ed_fm_change_mass  (double & delta_mass,
												double & delta_mass_pos_x,
												double & delta_mass_pos_y,
												double & delta_mass_pos_z,
												double & delta_mass_moment_of_inertia_x,
												double & delta_mass_moment_of_inertia_y,
												double & delta_mass_moment_of_inertia_z
												);
	// Internal fuel callbacks.
	F_CK_1C_EFM_API void   ed_fm_set_internal_fuel(double fuel);
	F_CK_1C_EFM_API double ed_fm_get_internal_fuel();
	// External fuel callbacks.
	F_CK_1C_EFM_API void  ed_fm_set_external_fuel (int	 station,
														double fuel,
														double x,
														double y,
														double z);
	F_CK_1C_EFM_API double ed_fm_get_external_fuel ();

	F_CK_1C_EFM_API void ed_fm_set_draw_args (EdDrawArgument * drawargs,size_t size);
	F_CK_1C_EFM_API void ed_fm_configure		(const char * cfg_path);


	F_CK_1C_EFM_API double ed_fm_get_param(unsigned index);

	// Starting-condition callbacks.
	F_CK_1C_EFM_API void ed_fm_cold_start();
	F_CK_1C_EFM_API void ed_fm_hot_start();
	F_CK_1C_EFM_API void ed_fm_hot_start_in_air();

	F_CK_1C_EFM_API bool ed_fm_enable_debug_info();
	F_CK_1C_EFM_API size_t ed_fm_debug_watch(int level, char *buffer, size_t maxlen);
	F_CK_1C_EFM_API void ed_fm_suspension_feedback(int idx, const ed_fm_suspension_info * info);

	F_CK_1C_EFM_API bool ed_fm_pop_simulation_event(ed_fm_simulation_event& out);
	F_CK_1C_EFM_API bool ed_fm_push_simulation_event(const ed_fm_simulation_event& in);
};

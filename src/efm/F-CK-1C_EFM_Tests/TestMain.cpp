#include "TestHarness.h"

void run_common_tests(Tests::Context& context);
void run_engine_system_tests(Tests::Context& context);
void run_fm_data_tests(Tests::Context& context);
void run_input_system_tests(Tests::Context& context);

int main()
{
	Tests::Context context;
	run_common_tests(context);
	run_input_system_tests(context);
	run_engine_system_tests(context);
	run_fm_data_tests(context);
	return context.finish();
}

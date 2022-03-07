#include "cl_plc_objects.h"

//#include "clcsHMIobjects.h"

cl_ton::cl_ton(uint32_t _timeDelay)
{
	timeDelay = _timeDelay;
}

boolean cl_ton::process(boolean _input)
{

	// If the input is false, disable timer
	if (!_input)
	{
		timing = false;
		return false;
	}

	// If input is true, and timer is disabled, restart the timer
	else if (!timing && _input)
	{
		timing = true;
		ton = millis();
		return false;
	}

	//if input is true, and has been true for longer than timeDelay, TON is true
	else if (timing && _input && millis() - ton > timeDelay)
	{
		return true;
	}

	return false;
}

void cl_ton::set(uint32_t _time)
{
	timeDelay = _time;
}

boolean cl_ton::get(void)
{
	return timing && millis() - ton > timeDelay;
}

cl_timer::cl_timer(uint32_t _time)
{
	timer = _time;
}

boolean cl_timer::check()
{
	if (millis() - start_time > timer)
		return true;
	else
		return false;
}
void cl_timer::reset()
{
	start_time = millis();
}

void cl_timer::set(uint32_t _time)
{
	timer = _time;
}

cl_debounce::cl_debounce(int _pin)
{
	pin = _pin;
	pinMode(pin, INPUT);
}
bool cl_debounce::getInput()
{
	state_off.process(!digitalRead(pin));
	state_on.process(digitalRead(pin));
	state.set(state_on.get());
	state.reset(state_off.get());
	return state.state;
}

bool cl_debounce::getState()
{
	return state.state;
}

cl_r_trig::cl_r_trig()
{
	trig_state = false;
}
bool cl_r_trig::process(bool _input)
{
	if (_input && !prev_state)
		trig_state = true;
	else
		trig_state = false;

	prev_state = _input;

	return trig_state;
}
bool cl_r_trig::get()
{
	return trig_state;
}

cl_timer_hms::cl_timer_hms()
{
}
int cl_timer_hms::getHour()
{
	return (millis() - t_start) / (60 * 60 * 1000);
}
int cl_timer_hms::getMin()
{
	return ((millis() - t_start) / 60000) % (60);
}
int cl_timer_hms::getSec()
{
	return ((millis() - t_start) / 1000) % 60;
}
bool cl_timer_hms::start()
{
	t_start = millis();
}

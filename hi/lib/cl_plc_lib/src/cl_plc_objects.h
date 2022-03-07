#ifndef CLCSPLCFUNCTIONS
#define CLCSPLCFUNCTIONS

#include "arduino.h"
//#include "clcsStructs.h"

struct cl_stopwatch
{
	//boolean timing;
	uint32_t start_time;
	void start()
	{
		start_time = millis();
	}

	uint32_t elapsed()
	{
		return millis() - start_time;
	}
};

// --------------------------------------------------------
// Latching Structure
// --------------------------------------------------------
//
// The latching structure maintains its state until a positive value
// is passed to set or reset.

struct cl_latch
{
	boolean state;
	void set(boolean _check)
	{
		if (_check)
			state = true;
	}
	void reset(boolean _check)
	{
		if (_check)
			state = false;
	}
};

// --------------------------------------------------------
// Timer on delay
// --------------------------------------------------------
//
// The TON structure will process the time an input is true. If
// the input has been in a true state for longer than timeDelay,
// the state of the TON goes true. It resets as soon as the input to
// process is false;

class cl_ton
{
public:
	cl_ton();
	cl_ton(uint32_t _timeDelay);
	boolean process(boolean _input);
	void set(uint32_t _time);
	boolean get(void);

private:
	boolean timing;
	uint32_t ton;
	uint32_t timeDelay;
};

// --------------------------------------------------------
// Timer
// --------------------------------------------------------
// The timer structure will start timing right after reset
// is called. The state of timer will turn true when the time
// elapsed is grater than timer.

class cl_timer
{
public:
	cl_timer(uint32_t _time);
	boolean check();
	void reset();
	void set(uint32_t _time);
	uint32_t start_time;
	uint32_t timer;

private:
	//uint32_t start_time;
};

// --------------------------------------------------------
// Debounce
// --------------------------------------------------------
//
// The debounce structure will filter out high frequency input changes.
// In this case, the input needs to be in a certain state for at least 5ms
// before the state will change.

class cl_debounce
{
public:
	cl_debounce(int _pin);
	bool getInput();
	bool getState();

private:
	int pin;
	cl_ton state_off = cl_ton(5);
	cl_ton state_on = cl_ton(5);
	cl_latch state;
};

// --------------------------------------------------------
// Rising Edge trigger
// --------------------------------------------------------
//
// A rising edge trigger state is true when the input to processing
// goes from LOW to HIGH. Processing returns the result of detecting
// a rising edge. The GET function will get the state of the previous
// processing without interfering with the state.

class cl_r_trig
{
public:
	cl_r_trig();
	bool process(bool _input);
	bool get();

private:
	bool prev_state;
	bool trig_state;
};

class cl_timer_hms
{
public:
	cl_timer_hms();
	int getHour();
	int getMin();
	int getSec();
	bool start();

private:
	uint32_t t_start;
};

#endif

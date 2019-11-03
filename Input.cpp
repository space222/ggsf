#include "pch.h"
#include <sol/sol.hpp>

// Input could be a SystemInterface that has no components
// but so far, for how simply it is used here, that's
// just not necessary.

struct cont_input
{
	bool valid;
	int buttons;
	unsigned char Ltrigger, Rtrigger;
	float LThumbX, LThumbY, Ldist;
	float RThumbX, RThumbY, Rdist;
};

cont_input getControllerState(int index)
{
	XINPUT_STATE st;
	auto res = XInputGetState(index, &st);
	if (res != ERROR_SUCCESS) return { false };

	XINPUT_GAMEPAD gp = st.Gamepad;
	cont_input I;
	I.valid = true;
	I.buttons = gp.wButtons;
	I.Ltrigger = gp.bLeftTrigger;
	I.Rtrigger = gp.bRightTrigger;

	float Lx(gp.sThumbLX);
	float Ly(gp.sThumbLY);
	float LM(sqrtf(Lx * Lx + Ly * Ly));
	I.LThumbX = Lx / LM;
	I.LThumbY = Ly / LM;
	if (LM > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
	{
		LM = (LM > 32767) ? 32767 : LM;
		LM -= XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
		I.Ldist = (LM / (32767 - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE));
	} else {
		I.Ldist = 0.0f;
	}

	float Rx(gp.sThumbRX);
	float Ry(gp.sThumbRY);
	float RM(sqrtf(Rx * Rx + Ry * Ry));
	I.RThumbX = Rx / RM;
	I.RThumbY = Ry / RM;
	if (RM > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
	{
		RM = (RM > 32767) ? 32767 : RM;
		RM -= XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
		I.Rdist = (RM / (32767 - XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE));
	} else {
		I.Rdist = 0.0f;
	}

	return I;
}

void setControllerVibration(int index, int left, int right)
{
	XINPUT_VIBRATION vib;
	vib.wLeftMotorSpeed =(WORD) left;
	vib.wRightMotorSpeed =(WORD)right;
	XInputSetState(index, &vib);
	return;
}

void input_init(sol::state& lua)
{
	lua.new_usertype<cont_input>("input_state", "valid", &cont_input::valid,
		"Ltrigger", &cont_input::Ltrigger, "Rtrigger", &cont_input::Rtrigger,
		"LThumbX", &cont_input::LThumbX, "LThumbY", &cont_input::LThumbY,
		"RThumbX", &cont_input::RThumbX, "RThumbY", &cont_input::RThumbY,
		"Rdist", &cont_input::Rdist, "Ldist", &cont_input::Ldist,
				"buttons", &cont_input::buttons);

	lua.set_function("getControllerState", getControllerState);
	lua.set_function("setControllerVibration", setControllerVibration);
	return;
}

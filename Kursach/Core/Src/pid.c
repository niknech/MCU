#include "pid.h"

void PID_Init(PID_Settings_t* pid_settings, PID_Control_t* pid_control, float* input_parameter)
{
  pid_settings->kp = 15.0f;
  pid_settings->ki = 25.0f;
  pid_settings->kd = 0.0f;
  pid_settings->setpoint = 7.42f;
  pid_settings->input = input_parameter;

  pid_control->dT = 0;
  pid_control->derivative = 0;
  pid_control->error = 0;
  pid_control->prev_error = 0;
  pid_control->integral = 0;
  pid_control->output = 0;
}

void PID_Update(PID_Control_t* pc, PID_Settings_t ps)
{
    if (pc->dT <= 0.0f)
        return;

    pc->error = ps.setpoint - *(ps.input);

    float P = ps.kp * pc->error;

    if (ps.ki > 0.0f)
    {
        // Ограничение накопленного интеграла
        float max_integral = 100.0f / ps.ki;

        if (pc->integral > max_integral)
            pc->integral = max_integral;

        if (pc->integral < 0.0f)
            pc->integral = 0.0f;

        float new_integral =
            pc->integral + pc->error * pc->dT;

        if (new_integral > max_integral)
            new_integral = max_integral;

        if (new_integral < 0.0f)
            new_integral = 0.0f;

        float new_output =
            P + ps.ki * new_integral;

        // Anti-windup
        if (!((new_output > 100.0f && pc->error > 0.0f) ||
              (new_output < 0.0f && pc->error < 0.0f)))
        {
            pc->integral = new_integral;
        }
    }
    else
    {
        // При отключённом I не накапливаем интеграл
        pc->integral = 0.0f;
    }

    pc->output = P + ps.ki * pc->integral;

    if (pc->output > 100.0f)
        pc->output = 100.0f;

    if (pc->output < 0.0f)
        pc->output = 0.0f;
}

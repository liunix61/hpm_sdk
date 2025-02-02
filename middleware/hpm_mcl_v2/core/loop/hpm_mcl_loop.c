/*
 * Copyright (c) 2023-2024 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "hpm_mcl_loop.h"

hpm_mcl_stat_t hpm_mcl_loop_init(mcl_loop_t *loop, mcl_loop_cfg_t *cfg, mcl_cfg_t *mcl_cfg,
                                mcl_encoder_t *encoder, mcl_analog_t *analog,
                                mcl_control_t *control, mcl_drivers_t *drivers, mcl_path_plan_t *path)
{
    MCL_ASSERT(loop != NULL, mcl_invalid_pointer);
    MCL_ASSERT(cfg != NULL, mcl_invalid_pointer);
    MCL_ASSERT(mcl_cfg != NULL, mcl_invalid_pointer);
    if (cfg->mode != mcl_mode_step_foc) {
        MCL_ASSERT(encoder != NULL, mcl_invalid_pointer);
    } else {
        MCL_ASSERT(path != NULL, mcl_invalid_pointer);
    }
    MCL_ASSERT(analog != NULL, mcl_invalid_pointer);
    MCL_ASSERT(control != NULL, mcl_invalid_pointer);
    MCL_ASSERT(drivers != NULL, mcl_invalid_pointer);

    loop->enable = false;
    loop->cfg = cfg;
    loop->path = path;
    loop->status = loop_status_run;
#ifndef HPM_MCL_Q_EN
    loop->const_vbus = &mcl_cfg->physical.motor.vbus;
    loop->ld = &mcl_cfg->physical.motor.ld;
    loop->lq = &mcl_cfg->physical.motor.lq;
    loop->flux = &mcl_cfg->physical.motor.flux;
#else
    loop->const_vbus = &mcl_cfg->physical_q.motor.vbus;
    loop->ld = &mcl_cfg->physical_q.motor.ld;
    loop->lq = &mcl_cfg->physical_q.motor.lq;
    loop->flux = &mcl_cfg->physical_q.motor.flux;
#endif
#if defined(MCL_CFG_EN_SENSORLESS_SMC) && MCL_CFG_EN_SENSORLESS_SMC
    MCL_ASSERT(encoder != NULL, mcl_invalid_pointer);
    encoder->sensorless.enable = &loop->cfg->enable_smc;
    encoder->sensorless.theta = &control->cfg->smc_cfg.theta;
    control->cfg->smc_cfg.cfg.const_data.inductor = mcl_cfg->physical.motor.ls;
    control->cfg->smc_cfg.cfg.const_data.loop_ts = mcl_cfg->physical.time.current_loop_ts;
    control->cfg->smc_cfg.cfg.const_data.res = mcl_cfg->physical.motor.res;
    control->cfg->smc_cfg.cfg.const_data.sample_ts = mcl_cfg->physical.time.adc_sample_ts;
    control->method.smc_init(&control->cfg->smc_cfg);
#endif
    loop->const_time.current_ts = &mcl_cfg->physical.time.current_loop_ts;
    loop->const_time.position_ts = &mcl_cfg->physical.time.speed_loop_ts;
    loop->const_time.speed_ts = &mcl_cfg->physical.time.speed_loop_ts;
    loop->const_time.dead_area_ts = (float)mcl_cfg->physical.board.pwm_dead_time_tick / mcl_cfg->physical.time.pwm_clock_tick;
    loop->encoder = encoder;
    loop->analog = analog;
    loop->control = control;
    loop->drivers = drivers;
    loop->time.position_ts = 0;
    loop->time.speed_ts = 0;
    loop->exec_ref.id = 0;
    loop->exec_ref.iq = 0;
    loop->exec_ref.position = 0;
    loop->exec_ref.speed = 0;
    loop->rundata.block.dir = motor_dir_forward;

    hpm_mcl_loop_disable_all_user_set_value(loop);

    return mcl_success;
}

hpm_mcl_stat_t hpm_mcl_loop_set_current_d(mcl_loop_t *loop, mcl_user_value_t id)
{
    MCL_ASSERT_OPT(loop != NULL, mcl_invalid_pointer);
    loop->ref_id = id;
    return mcl_success;
}

hpm_mcl_stat_t hpm_mcl_loop_set_current_q(mcl_loop_t *loop, mcl_user_value_t iq)
{
    MCL_ASSERT_OPT(loop != NULL, mcl_invalid_pointer);
    loop->ref_iq = iq;
    return mcl_success;
}

hpm_mcl_stat_t hpm_mcl_loop_set_speed(mcl_loop_t *loop, mcl_user_value_t speed)
{
    MCL_ASSERT_OPT(loop != NULL, mcl_invalid_pointer);
    loop->ref_speed = speed;
    return mcl_success;
}

hpm_mcl_stat_t hpm_mcl_loop_set_position(mcl_loop_t *loop, mcl_user_value_t position)
{
    MCL_ASSERT_OPT(loop != NULL, mcl_invalid_pointer);
    loop->ref_position = position;
    return mcl_success;
}

hpm_mcl_stat_t hpm_mcl_loop_disable_all_user_set_value(mcl_loop_t *loop)
{
    MCL_ASSERT_OPT(loop != NULL, mcl_invalid_pointer);
    loop->ref_id.enable = false;
    loop->ref_iq.enable = false;
    loop->ref_position.enable = false;
    loop->ref_speed.enable = false;
    return mcl_success;
}


hpm_mcl_stat_t hpm_mcl_current_foc_loop(mcl_loop_t *loop)
{
    float ref_speed = 0;
    float sens_speed;
    float ref_position = 0;
    float ia, ib, ic;
    float theta;
    float alpha, beta;
    float sinx, cosx;
    float sens_d, sens_q;
    float ref_d = 0, ref_q = 0;
    float ud, uq;
    float theta_abs;
    mcl_hardware_clc_cfg_t *clc_cfg;
    mcl_control_svpwm_duty_t duty;
#if defined(MCL_CFG_EN_DEAD_AREA_COMPENSATION) && MCL_CFG_EN_DEAD_AREA_COMPENSATION
    mcl_control_dead_area_pwm_offset_t duty_offset;
#endif

#if defined(MCL_CFG_EN_THETA_FORECAST) && MCL_CFG_EN_THETA_FORECAST
    float sinx_, cosx_;
    float theta_forecast;
#endif

    MCL_ASSERT_OPT(loop != NULL, mcl_invalid_pointer);
    theta = hpm_mcl_encoder_get_theta(loop->encoder);
#if defined(MCL_CFG_EN_THETA_FORECAST) && MCL_CFG_EN_THETA_FORECAST
    theta_forecast = hpm_mcl_encoder_get_forecast_theta(loop->encoder);
#endif
    if (loop->enable) {
        if (loop->cfg->enable_position_loop) {
            loop->time.position_ts += *loop->const_time.current_ts;
            MCL_FUNCTION_SET_IF_ELSE_TRUE(loop->ref_position.enable, ref_position, loop->ref_position.value, loop->exec_ref.position);
            if (loop->time.position_ts >= *loop->const_time.position_ts) {
                loop->time.position_ts = 0;
                MCL_ASSERT_EXEC_CODE_AND_RETURN(hpm_mcl_encoder_get_absolute_theta(loop->encoder, &theta_abs) == mcl_success,
                loop->status = loop_status_fail, mcl_fail);
                loop->control->method.position_pid(ref_position, theta_abs, &loop->control->cfg->position_pid_cfg, &loop->exec_ref.speed);
            }
        } else {
            loop->exec_ref.speed = 0;
            loop->time.position_ts = 0;
        }
        if (loop->cfg->enable_speed_loop) {
            loop->time.speed_ts += *loop->const_time.current_ts;
            MCL_FUNCTION_SET_IF_ELSE_TRUE(loop->ref_speed.enable, ref_speed, loop->ref_speed.value, loop->exec_ref.speed);
            if (loop->time.speed_ts >= *loop->const_time.speed_ts) {
                loop->time.speed_ts = 0;
                sens_speed = hpm_mcl_encoder_get_speed(loop->encoder);
                loop->control->method.speed_pid(ref_speed, sens_speed,
                &loop->control->cfg->speed_pid_cfg, &loop->exec_ref.iq);
            }
        } else {
            loop->time.speed_ts = 0;
            loop->exec_ref.iq = 0;
        }
        MCL_VALUE_SET_IF_TRUE(loop->ref_id.enable, ref_d, loop->ref_id.value);
        MCL_FUNCTION_SET_IF_ELSE_TRUE(loop->ref_iq.enable, ref_q, loop->ref_iq.value, loop->exec_ref.iq);
        switch (loop->cfg->mode) {
        case mcl_mode_foc:
            /**
             * @brief current sample
             *
             */
            MCL_STATUS_SET_IF_TRUE(mcl_success != hpm_mcl_analog_get_value(loop->analog, analog_a_current, &ia), loop->status, loop_status_fail);
            MCL_STATUS_SET_IF_TRUE(mcl_success != hpm_mcl_analog_get_value(loop->analog, analog_b_current, &ib), loop->status, loop_status_fail);
            ic = -(ia + ib);
            loop->control->method.clarke(ia, ib, ic, &alpha, &beta);
#if defined(MCL_CFG_EN_SENSORLESS_SMC) && MCL_CFG_EN_SENSORLESS_SMC
            loop->control->method.smc_process(&loop->control->cfg->smc_cfg, loop->control->cfg->smc_cfg.ualpha,
                loop->control->cfg->smc_cfg.ubeta, alpha, beta);
#endif
            sinx = loop->control->method.sin_x(theta);
            cosx = loop->control->method.cos_x(theta);
            loop->control->method.park(alpha, beta, sinx, cosx, &sens_d, &sens_q);
            loop->control->method.currentd_pid(ref_d, sens_d, &loop->control->cfg->currentd_pid_cfg, &ud);
            loop->control->method.currentq_pid(ref_q, sens_q, &loop->control->cfg->currentq_pid_cfg, &uq);
#if defined(MCL_CFG_EN_DQ_AXIS_DECOUPLING) && MCL_CFG_EN_DQ_AXIS_DECOUPLING
            if (loop->cfg->enable_dq_axis_decoupling) {
                if (loop->cfg->enable_speed_loop) {
                    ud -= sens_q * sens_speed * (*loop->encoder->pole_num) * (*loop->lq);
                    uq += sens_speed * (*loop->encoder->pole_num) * (*loop->ld * sens_q + *loop->flux);
                }
            }
#endif
#if defined(MCL_CFG_EN_THETA_FORECAST) && MCL_CFG_EN_THETA_FORECAST
            sinx_ = loop->control->method.sin_x(theta_forecast);
            cosx_ = loop->control->method.cos_x(theta_forecast);
            loop->control->method.invpark(ud, uq, sinx_, cosx_, &alpha, &beta);
#else
            loop->control->method.invpark(ud, uq, sinx, cosx, &alpha, &beta);
#endif

#if defined(MCL_CFG_EN_SENSORLESS_SMC) && MCL_CFG_EN_SENSORLESS_SMC
            loop->control->cfg->smc_cfg.ualpha = alpha;
            loop->control->cfg->smc_cfg.ubeta = beta;
#endif
            loop->control->method.svpwm(alpha, beta, *loop->const_vbus, &duty);
#if defined(MCL_CFG_EN_DEAD_AREA_COMPENSATION) && MCL_CFG_EN_DEAD_AREA_COMPENSATION
            if (loop->cfg->enable_dead_area_compensation) {
                loop->control->method.dead_area_polarity_detection(&loop->control->cfg->dead_area_compensation_cfg, sens_d, sens_q, theta, loop->const_time.dead_area_ts,
                                                                    *loop->const_time.current_ts, &duty_offset);
                duty.a += duty_offset.a_offset;
                duty.b += duty_offset.b_offset;
                duty.c += duty_offset.c_offset;
            }
#endif
            hpm_mcl_drivers_update_bldc_duty(loop->drivers, duty.a, duty.b, duty.c);

            break;
        case mcl_mode_hardware_foc:
            clc_cfg = (mcl_hardware_clc_cfg_t *)loop->hardware;
            clc_cfg->clc_set_val(loop_chn_id, clc_cfg->convert_float_to_clc_val(ref_d));
            clc_cfg->clc_set_val(loop_chn_iq, clc_cfg->convert_float_to_clc_val(ref_q));
            break;
        default:
            break;
        }
    } else {
        duty.a = 0;
        duty.b = 0;
        duty.c = 0;
        hpm_mcl_drivers_update_bldc_duty(loop->drivers, duty.a, duty.b, duty.c);
    }
    return  mcl_success;
}

hpm_mcl_stat_t hpm_mcl_step_foc_loop(mcl_loop_t *loop)
{
    float ia, ib;
    float theta;
    float alpha, beta;
    float sinx, cosx;
    float sens_d, sens_q;
    float ref_d = 0;
    float ud, uq;
    float sinx_, cosx_;
    float theta_;
    mcl_control_svpwm_duty_t duty;

    MCL_ASSERT_OPT(loop != NULL, mcl_invalid_pointer);
    theta =  hpm_mcl_path_get_current_theta(loop->path);
    theta_ = MCL_ANGLE_MOD_X(0, MCL_PI * 2, (hpm_mcl_path_get_next_theta(loop->path) - (0.25f * MCL_PI)));

    if (loop->enable) {
        MCL_STATUS_SET_IF_TRUE(mcl_success != hpm_mcl_analog_get_value(loop->analog, analog_a_current, &ia), loop->status, loop_status_fail);
        MCL_STATUS_SET_IF_TRUE(mcl_success != hpm_mcl_analog_get_value(loop->analog, analog_b_current, &ib), loop->status, loop_status_fail);
        MCL_STATUS_SET_IF_TRUE(mcl_success != hpm_mcl_analog_step_convert(loop->analog, ia, analog_a_current, theta, &ia), loop->status, loop_status_fail);
        MCL_STATUS_SET_IF_TRUE(mcl_success != hpm_mcl_analog_step_convert(loop->analog, ib, analog_b_current, theta, &ib), loop->status, loop_status_fail);

        theta =  MCL_ANGLE_MOD_X(0, MCL_PI * 2, (theta - (0.25f * MCL_PI)));
        MCL_VALUE_SET_IF_TRUE(loop->ref_id.enable, ref_d, loop->ref_id.value);
        alpha = ia;
        beta = ib;
        sinx = loop->control->method.sin_x(theta);
        cosx = loop->control->method.cos_x(theta);
        loop->control->method.park(alpha, beta, sinx, cosx, &sens_d, &sens_q);
        loop->control->method.currentd_pid(ref_d, sens_d, &loop->control->cfg->currentd_pid_cfg, &ud);
        loop->control->method.currentq_pid(0, sens_q, &loop->control->cfg->currentq_pid_cfg, &uq);
        sinx_ = loop->control->method.sin_x(theta_);
        cosx_ = loop->control->method.cos_x(theta_);
        loop->control->method.invpark(ud, uq, sinx_, cosx_, &alpha, &beta);
        loop->control->method.step_svpwm(alpha, beta, *loop->const_vbus, &duty);
        hpm_mcl_drivers_update_step_duty(loop->drivers, duty.a0, duty.a1, duty.b0, duty.b1);
    } else {
        duty.a0 = 0;
        duty.a1 = 0;
        duty.b0 = 0;
        duty.b1 = 0;
        hpm_mcl_drivers_update_step_duty(loop->drivers, duty.a0, duty.a1, duty.b0, duty.b1);
    }
    return  mcl_success;
}

hpm_mcl_stat_t hpm_mcl_block_loop(mcl_loop_t *loop)
{
    hpm_mcl_type_t speed_pi_out;
    float speed_pi_out_fp;
    float ref_speed = 0;
    mcl_control_svpwm_duty_t duty;

    MCL_ASSERT_OPT(loop != NULL, mcl_invalid_pointer);
    if (loop->enable) {
        if (loop->cfg->enable_position_loop) {
            MCL_ASSERT_EXEC_CODE_AND_RETURN(false, loop->status = loop_status_fail, mcl_invalid_argument);
        }
        if (loop->cfg->enable_speed_loop) {
            loop->time.speed_ts += *loop->const_time.speed_ts;
            MCL_FUNCTION_SET_IF_ELSE_TRUE(loop->ref_speed.enable, ref_speed, loop->ref_speed.value, loop->exec_ref.speed);
            if (loop->time.speed_ts >= *loop->const_time.speed_ts) {
                loop->time.speed_ts = 0;
                loop->control->method.speed_pid(ref_speed, hpm_mcl_encoder_get_speed(loop->encoder),
                &loop->control->cfg->speed_pid_cfg, &speed_pi_out);
            }
        } else {
            MCL_ASSERT_EXEC_CODE_AND_RETURN(false, loop->status = loop_status_fail, mcl_invalid_argument);
        }
        speed_pi_out_fp = MCL_MATH_CONVERT_FLOAT(speed_pi_out);
        if (speed_pi_out_fp < 0) {
            speed_pi_out_fp = -speed_pi_out_fp;
            loop->rundata.block.dir = motor_dir_forward;
        } else {
            loop->rundata.block.dir = motor_dir_back;
        }
        duty.a = speed_pi_out_fp;
        duty.b = speed_pi_out_fp;
        duty.c = speed_pi_out_fp;
        hpm_mcl_drivers_update_bldc_duty(loop->drivers, duty.a, duty.b, duty.c);
    } else {
        duty.a = 0;
        duty.b = 0;
        duty.c = 0;
        hpm_mcl_drivers_update_bldc_duty(loop->drivers, duty.a, duty.b, duty.c);
    }
    return  mcl_success;
}

hpm_mcl_stat_t hpm_mcl_loop_refresh_block(mcl_loop_t *loop)
{
    uint8_t sector;
    mcl_encoder_uvw_level_t level;

    hpm_mcl_encoder_get_uvw_status(loop->encoder, &level);
    loop->control->method.get_block_sector(*loop->encoder->phase, level.u, level.v, level.w, &sector);
    hpm_mcl_drivers_block_update(loop->drivers, loop->rundata.block.dir, sector);

    return mcl_success;
}

hpm_mcl_stat_t hpm_mcl_loop_start_block(mcl_loop_t *loop)
{
    uint8_t sector;
    mcl_encoder_uvw_level_t level;

    hpm_mcl_encoder_get_uvw_status(loop->encoder, &level);
    loop->control->method.get_block_sector(*loop->encoder->phase, level.u, level.v, level.w, &sector);
    hpm_mcl_drivers_block_update(loop->drivers, loop->rundata.block.dir, ((sector + 1) % 7) + 1);

    return mcl_success;
}

hpm_mcl_stat_t hpm_mcl_loop(mcl_loop_t *loop)
{
    MCL_ASSERT_OPT(loop != NULL, mcl_invalid_pointer);
    switch (loop->cfg->mode) {
    case mcl_mode_foc:
    case mcl_mode_hardware_foc:
        return hpm_mcl_current_foc_loop(loop);
    case mcl_mode_block:
        return hpm_mcl_block_loop(loop);
        break;
    case mcl_mode_step_foc:
        return hpm_mcl_step_foc_loop(loop);
        break;
    default:
        MCL_ASSERT_EXEC_CODE_AND_RETURN(false, loop->status = loop_status_fail, mcl_fail);
        break;
    }
    return mcl_fail;
}

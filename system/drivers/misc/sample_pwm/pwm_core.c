/*
 *  Copyright (C) 2014,  King liuyang <liuyang@ingenic.cn>
 *  jz_PWM support
 */

#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/mfd/core.h>
#include <linux/mfd/jz_tcu.h>
#include <soc/extal.h>
#include <soc/gpio.h>
#include <linux/slab.h>

#if defined(CONFIG_SOC_T30) || defined(CONFIG_SOC_T21)
#define PWM_NUM		NR_TCU_CHNS
#else /* other soc type */
#define PWM_NUM		4
#endif

#define PWM_DRIVER_VERSION "H20180309a"
struct jz_pwm_device{
	short id;
	const char *label;
	struct jz_tcu_chn *tcu_cha;
};

struct jz_pwm_chip {
	struct device *dev;
	const struct mfd_cell *cell;

	struct jz_pwm_device *pwm_chrs;
	struct pwm_chip chip;
};

static inline struct jz_pwm_chip *to_jz(struct pwm_chip *chip)
{
	return container_of(chip, struct jz_pwm_chip, chip);
}

static int jz_pwm_request(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct jz_pwm_chip *jz = to_jz(chip);
	int id = jz->pwm_chrs->tcu_cha->index;
	if (id < 0 || id > PWM_NUM)
		return -ENODEV;

	printk("request pwm channel %d successfully\n", id);

	return 0;
}

static void jz_pwm_free(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct jz_pwm_chip *jz = to_jz(chip);
	kfree(jz->pwm_chrs);
	kfree (jz);
}

static int jz_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct jz_pwm_chip *jz = to_jz(chip);
	struct jz_tcu_chn *tcu_pwm = jz->pwm_chrs->tcu_cha;

	jz_tcu_start_counter(tcu_pwm);

	jz_tcu_enable_counter(tcu_pwm);

#if defined(CONFIG_SOC_T21)
	if(tcu_pwm->index == 1 || tcu_pwm->index == 0)
		jzgpio_set_func(tcu_pwm->gpio / 32,GPIO_FUNC_0,BIT(tcu_pwm->gpio & 0x1f));
	else
		jzgpio_set_func(tcu_pwm->gpio / 32,GPIO_FUNC_1,BIT(tcu_pwm->gpio & 0x1f));
#elif defined(CONFIG_SOC_T31)
	if(tcu_pwm->gpio==(32+27)||tcu_pwm->gpio==(32+28)){
		jzgpio_set_func(tcu_pwm->gpio / 32,GPIO_FUNC_1,BIT(tcu_pwm->gpio & 0x1f));
	}
	else{
		jzgpio_set_func(tcu_pwm->gpio / 32,GPIO_FUNC_0,BIT(tcu_pwm->gpio & 0x1f));
	}
#else
	jzgpio_set_func(tcu_pwm->gpio / 32,GPIO_FUNC_0,BIT(tcu_pwm->gpio & 0x1f));
#endif
	return 0;
}

static void jz_pwm_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct jz_pwm_chip *jz = to_jz(chip);
	struct jz_tcu_chn *tcu_pwm = jz->pwm_chrs->tcu_cha;

	jz_tcu_disable_counter(tcu_pwm);

	jz_tcu_stop_counter(tcu_pwm);
	if(tcu_pwm->index == 1 || tcu_pwm->index == 2)
		jzgpio_set_func(tcu_pwm->gpio / 32,tcu_pwm->init_level > 0 ? GPIO_OUTPUT1 : GPIO_OUTPUT0,BIT(tcu_pwm->gpio & 0x1f));
}

static int jz_pwm_config(struct pwm_chip *chip, struct pwm_device *pwm,
			     int duty_ns, int period_ns)
{
	unsigned long long tmp;
	unsigned long period, duty;
	struct jz_pwm_chip *jz = to_jz(chip);
	int prescaler = 0; /*prescale = 0,1,2,3,4,5*/

	struct jz_tcu_chn *tcu_pwm = jz->pwm_chrs->tcu_cha;

	if (duty_ns < 0 || duty_ns > period_ns)
		return -EINVAL;
	if (period_ns < 200 || period_ns > 1000000000)
		return -EINVAL;

	tcu_pwm->irq_type = NULL_IRQ_MODE;
	tcu_pwm->is_pwm = pwm->flags;

#ifndef CONFIG_SLCD_SUSPEND_ALARM_WAKEUP_REFRESH
	tmp = JZ_EXTAL;
#else
	tmp = JZ_EXTAL;//32768 RTC CLOCK failure!
#endif
	tmp = tmp * period_ns;

	do_div(tmp, 1000000000);
	period = tmp;

	while (period > 0xffff && prescaler < 6) {
		period >>= 2;
		++prescaler;
	}
	if (prescaler == 6)
		return -EINVAL;

	tmp = (unsigned long long)period * duty_ns;
	do_div(tmp, period_ns);
	duty = tmp;

	if (duty >= period)
		duty = period - 1;
	tcu_pwm->full_num = period;
	tcu_pwm->half_num = duty;
	tcu_pwm->prescale = prescaler;
#ifdef CONFIG_SLCD_SUSPEND_ALARM_WAKEUP_REFRESH
	tcu_pwm->clk_src = TCU_CLKSRC_RTC;
#else
	tcu_pwm->clk_src = TCU_CLKSRC_EXT;
#endif
	tcu_pwm->shutdown_mode = 0;

	jz_tcu_config_chn(tcu_pwm);

	jz_tcu_set_period(tcu_pwm,tcu_pwm->full_num);

	jz_tcu_set_duty(tcu_pwm,tcu_pwm->half_num);
//	printk("hwpwm = %d index = %d\n", pwm->hwpwm, tcu_pwm->index);

	return 0;
}

int jz_pwm_set_polarity(struct pwm_chip *chip,struct pwm_device *pwm, enum pwm_polarity polarity)
{
	struct jz_pwm_chip *jz = to_jz(chip);
	struct jz_tcu_chn *tcu_pwm = jz->pwm_chrs->tcu_cha;

	if(polarity == PWM_POLARITY_INVERSED)
		tcu_pwm->init_level = 0;

	if(polarity == PWM_POLARITY_NORMAL)
		tcu_pwm->init_level = 1;

	return 0;
}

static const struct pwm_ops jz_pwm_ops = {
	.request = jz_pwm_request,
	.free = jz_pwm_free,
	.config = jz_pwm_config,
	.set_polarity = jz_pwm_set_polarity,
	.enable = jz_pwm_enable,
	.disable = jz_pwm_disable,
	.owner = THIS_MODULE,
};

static int jz_pwm_probe(struct platform_device *pdev)
{
	struct jz_pwm_chip *jz;
	int ret;

	jz = kzalloc(sizeof(struct jz_pwm_chip), GFP_KERNEL);

	jz->pwm_chrs = kzalloc(sizeof(struct jz_pwm_device),GFP_KERNEL);
	if (!jz || !jz->pwm_chrs)
		return -ENOMEM;

	jz->cell = mfd_get_cell(pdev);
	if (!jz->cell) {
		ret = -ENOENT;
		dev_err(&pdev->dev, "Failed to get mfd cell\n");
		goto err_free;
	}

	jz->chip.dev = &pdev->dev;
	jz->chip.ops = &jz_pwm_ops;
	jz->chip.npwm = PWM_NUM;
	jz->chip.base = -1;

	jz->pwm_chrs->tcu_cha = (struct jz_tcu_chn *)jz->cell->platform_data;

	ret = pwmchip_add(&jz->chip);
	if (ret < 0) {
		printk("%s[%d] ret = %d\n", __func__,__LINE__, ret);
		goto err_free;
	}

	if(pdev->dev.driver)
		printk("%s[%d] d_name = %s\n", __func__,__LINE__,pdev->dev.driver->name);
	platform_set_drvdata(pdev, jz);

	return 0;
err_free:
		kfree(jz->pwm_chrs);
		kfree(jz);
	return ret;
}

static int jz_pwm_remove(struct platform_device *pdev)
{
	struct jz_pwm_chip *jz = platform_get_drvdata(pdev);
	int ret;

	ret = pwmchip_remove(&jz->chip);
	if (ret < 0)
		return ret;


	return 0;
}

#ifdef CONFIG_PWM0
static struct platform_driver jz_pwm0_driver = {
	.driver = {
		.name = "tcu_chn0",
		.owner = THIS_MODULE,
	},
	.probe = jz_pwm_probe,
	.remove = jz_pwm_remove,
};
#endif

#ifdef CONFIG_PWM1
static struct platform_driver jz_pwm1_driver = {
	.driver = {
		.name = "tcu_chn1",
		.owner = THIS_MODULE,
	},
	.probe = jz_pwm_probe,
	.remove = jz_pwm_remove,
};
#endif

#ifdef CONFIG_PWM2
static struct platform_driver jz_pwm2_driver = {
	.driver = {
		.name = "tcu_chn2",
		.owner = THIS_MODULE,
	},
	.probe = jz_pwm_probe,
	.remove = jz_pwm_remove,
};
#endif

#ifdef CONFIG_PWM3
static struct platform_driver jz_pwm3_driver = {
	.driver = {
		.name = "tcu_chn3",
		.owner = THIS_MODULE,
	},
	.probe = jz_pwm_probe,
	.remove = jz_pwm_remove,
};
#endif

#if defined(CONFIG_SOC_T30) || defined(CONFIG_SOC_T21)
#if (PWM_NUM > 4)
#ifdef CONFIG_PWM4
static struct platform_driver jz_pwm4_driver = {
	.driver = {
		.name = "tcu_chn4",
		.owner = THIS_MODULE,
	},
	.probe = jz_pwm_probe,
	.remove = jz_pwm_remove,
};
#endif
#endif

#if (PWM_NUM > 5)
#ifdef CONFIG_PWM5
static struct platform_driver jz_pwm5_driver = {
	.driver = {
		.name = "tcu_chn5",
		.owner = THIS_MODULE,
	},
	.probe = jz_pwm_probe,
	.remove = jz_pwm_remove,
};
#endif
#endif

#if (PWM_NUM > 6)
#ifdef CONFIG_PWM6
static struct platform_driver jz_pwm6_driver = {
	.driver = {
		.name = "tcu_chn6",
		.owner = THIS_MODULE,
	},
	.probe = jz_pwm_probe,
	.remove = jz_pwm_remove,
};
#endif
#endif

#if (PWM_NUM > 7)
#ifdef CONFIG_PWM7
static struct platform_driver jz_pwm7_driver = {
	.driver = {
		.name = "tcu_chn7",
		.owner = THIS_MODULE,
	},
	.probe = jz_pwm_probe,
	.remove = jz_pwm_remove,
};
#endif
#endif
#else /* T20 */
#if (defined(CONFIG_PWM4) || defined(CONFIG_PWM5) || defined(CONFIG_PWM6) || defined(CONFIG_PWM7))
#error "Can't support the pwm in t10/t20 chip!"
#endif
#endif

static int __init pwm_init(void)
{
#ifdef CONFIG_PWM0
	platform_driver_register(&jz_pwm0_driver);
#endif
#ifdef CONFIG_PWM1
	platform_driver_register(&jz_pwm1_driver);
#endif
#ifdef CONFIG_PWM2
	platform_driver_register(&jz_pwm2_driver);
#endif
#ifdef CONFIG_PWM3
	platform_driver_register(&jz_pwm3_driver);
#endif
#ifdef CONFIG_PWM4
	platform_driver_register(&jz_pwm4_driver);
#endif
#ifdef CONFIG_PWM5
	platform_driver_register(&jz_pwm5_driver);
#endif
#ifdef CONFIG_PWM6
	platform_driver_register(&jz_pwm6_driver);
#endif
#ifdef CONFIG_PWM7
	platform_driver_register(&jz_pwm7_driver);
#endif
	printk("The version of PWM driver is %s\n", PWM_DRIVER_VERSION);
	return 0;
}

static void __exit pwm_exit(void)
{
#ifdef CONFIG_PWM0
	platform_driver_unregister(&jz_pwm0_driver);
#endif
#ifdef CONFIG_PWM1
	platform_driver_unregister(&jz_pwm1_driver);
#endif
#ifdef CONFIG_PWM2
	platform_driver_unregister(&jz_pwm2_driver);
#endif
#ifdef CONFIG_PWM3
	platform_driver_unregister(&jz_pwm3_driver);
#endif
#ifdef CONFIG_PWM4
	platform_driver_unregister(&jz_pwm4_driver);
#endif
#ifdef CONFIG_PWM5
	platform_driver_unregister(&jz_pwm5_driver);
#endif
#ifdef CONFIG_PWM6
	platform_driver_unregister(&jz_pwm6_driver);
#endif
#ifdef CONFIG_PWM7
	platform_driver_unregister(&jz_pwm7_driver);
#endif
}
#undef PWM_NUM
module_init(pwm_init);
module_exit(pwm_exit);

MODULE_ALIAS("platform:jz-pwm");
MODULE_LICENSE("GPL");

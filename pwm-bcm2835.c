/*
 * pwm-bcm2835 driver
 * Standard raspberry pi (gpio18 - pwm0)
 *
 * Copyright (C) 2014 Thomas more
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2.
 */

#include <linux/clk.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>

/*mmio regiser mapping*/

#define DUTY		0x14
#define PERIOD		0x10
#define CHANNEL		0x10

#define PWM_ENABLE	0x00000001
#define PWM_POLARITY	0x00000010

#define MASK_CTL_PWM	0x000000FF
#define CTL_PWM		0x00000081

#define DRIVER_AUTHOR "Bart Tanghe <bart.tanghe@thomasmore.be>"
#define DRIVER_DESC "A bcm2835 pwm driver - raspberry pi development platform"

struct bcm2835_pwm_chip {
	struct pwm_chip chip;
	struct device *dev;
	int channel;
	int scaler;
	void __iomem *mmio_base;
};

static inline struct bcm2835_pwm_chip *to_bcm2835_pwm_chip(
					struct pwm_chip *chip){

	return container_of(chip, struct bcm2835_pwm_chip, chip);
}

static int bcm2835_pwm_config(struct pwm_chip *chip,
			      struct pwm_device *pwm,
			      int duty_ns, int period_ns){

	struct bcm2835_pwm_chip *pc;

	pc = container_of(chip, struct bcm2835_pwm_chip, chip);

	iowrite32(duty_ns/pc->scaler, pc->mmio_base + DUTY);
	iowrite32(period_ns/pc->scaler, pc->mmio_base + PERIOD);

	return 0;
}

static int bcm2835_pwm_enable(struct pwm_chip *chip,
			      struct pwm_device *pwm){

	struct bcm2835_pwm_chip *pc;

	pc = container_of(chip, struct bcm2835_pwm_chip, chip);

	iowrite32(ioread32(pc->mmio_base) | PWM_ENABLE, pc->mmio_base);
	return 0;
}

static void bcm2835_pwm_disable(struct pwm_chip *chip,
				struct pwm_device *pwm)
{
	struct bcm2835_pwm_chip *pc;

	pc = to_bcm2835_pwm_chip(chip);

	iowrite32(ioread32(pc->mmio_base) & ~PWM_ENABLE, pc->mmio_base);
}

static int bcm2835_set_polarity(struct pwm_chip *chip, struct pwm_device *pwm,
				enum pwm_polarity polarity)
{
	struct bcm2835_pwm_chip *pc;

	pc = to_bcm2835_pwm_chip(chip);

	if (polarity == PWM_POLARITY_NORMAL)
		iowrite32((ioread32(pc->mmio_base) & ~PWM_POLARITY),
						pc->mmio_base);
	else if (polarity == PWM_POLARITY_INVERSED)
		iowrite32((ioread32(pc->mmio_base) | PWM_POLARITY),
						pc->mmio_base);

	return 0;
}

static const struct pwm_ops bcm2835_pwm_ops = {
	.config = bcm2835_pwm_config,
	.enable = bcm2835_pwm_enable,
	.disable = bcm2835_pwm_disable,
	.set_polarity = bcm2835_set_polarity,
	.owner = THIS_MODULE,
};

static int bcm2835_pwm_probe(struct platform_device *pdev)
{
	struct bcm2835_pwm_chip *pwm;

	int ret;
	struct resource *r;
	u32 start, end;
	struct clk *clk;

	pwm = devm_kzalloc(&pdev->dev, sizeof(*pwm), GFP_KERNEL);
	if (!pwm) {
		dev_err(&pdev->dev, "failed to allocate memory\n");
		return -ENOMEM;
	}

	pwm->dev = &pdev->dev;

	clk = clk_get(&pdev->dev, NULL);
	if (IS_ERR(clk)) {
		dev_err(&pdev->dev, "could not find clk: %ld\n", PTR_ERR(clk));
		devm_kfree(&pdev->dev, pwm);
		return PTR_ERR(clk);
	}

	pwm->scaler = (int)1000000000/clk_get_rate(clk);

	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	pwm->mmio_base = devm_ioremap_resource(&pdev->dev, r);
	if (IS_ERR(pwm->mmio_base))
	{
		devm_kfree(&pdev->dev, pwm);
		return PTR_ERR(pwm->mmio_base);
	}

	start = r->start;
	end = r->end;

	pwm->chip.dev = &pdev->dev;
	pwm->chip.ops = &bcm2835_pwm_ops;
	pwm->chip.npwm = 2;

	ret = pwmchip_add(&pwm->chip);
	if (ret < 0) {
		dev_err(&pdev->dev, "pwmchip_add() failed: %d\n", ret);
		devm_kfree(&pdev->dev, pwm);
		return -1;
	}

	/*set the pwm0 configuration*/
	iowrite32((ioread32(pwm->mmio_base) & ~MASK_CTL_PWM)
				| CTL_PWM, pwm->mmio_base);

	platform_set_drvdata(pdev, pwm);

	return 0;
}

static int bcm2835_pwm_remove(struct platform_device *pdev)
{

	struct bcm2835_pwm_chip *pc;
	pc  = platform_get_drvdata(pdev);

	if (WARN_ON(!pc))
		return -ENODEV;

	return pwmchip_remove(&pc->chip);
}

static const struct of_device_id bcm2835_pwm_of_match[] = {
	{ .compatible = "bcrm,pwm-bcm2835", },
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, bcm2835_pwm_of_match);

static struct platform_driver bcm2835_pwm_driver = {
	.driver = {
		.name = "pwm-bcm2835",
		.owner = THIS_MODULE,
		.of_match_table = bcm2835_pwm_of_match,
	},
	.probe = bcm2835_pwm_probe,
	.remove = bcm2835_pwm_remove,
};
module_platform_driver(bcm2835_pwm_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);

/*
 * ASoC driver for Milkymist chips
 *
 * Copyright (C) 2010 Sebastien Bourdeauducq
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/delay.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/ac97_codec.h>
#include <sound/initval.h>
#include <sound/soc.h>

#include <asm/hw/interrupts.h>
#include <asm/io.h>

#include "milkymist-ac97.h"
#include "milkymist-hw.h"

static unsigned short milkymist_ac97_read(struct snd_ac97 *ac97,
	unsigned short reg)
{
	int timeout;

	lm32_irq_ack(IRQ_AC97CRREPLY);
	iowrite32be((unsigned int)reg, CSR_AC97_CRADDR);
	iowrite32be(AC97_CRCTL_RQEN, CSR_AC97_CRCTL);

	timeout = 1000;
	while (!(lm32_irq_pending() & (1 << IRQ_AC97CRREPLY))) {
		udelay(10);
		timeout--;
		if(timeout == 0) {
			pr_warning("Timeout waiting for readout of AC97 register %02x\n", reg);
			return 0;
		}
	}
	lm32_irq_ack(IRQ_AC97CRREPLY);
	return ioread32be(CSR_AC97_CRDATAIN);
}

void milkymist_ac97_write(struct snd_ac97 *ac97, unsigned short reg,
	unsigned short val)
{
	int timeout;

	lm32_irq_ack(IRQ_AC97CRREQUEST);
	iowrite32be((unsigned int)reg, CSR_AC97_CRADDR);
	iowrite32be((unsigned int)val, CSR_AC97_CRDATAOUT);
	iowrite32be(AC97_CRCTL_RQEN | AC97_CRCTL_WRITE, CSR_AC97_CRDATAOUT);

	timeout = 1000;
	while (!(lm32_irq_pending() & (1 << IRQ_AC97CRREQUEST))) {
		udelay(10);
		timeout--;
		if(timeout == 0) {
			pr_warning("Timeout waiting for writing of AC97 register %02x\n", reg);
			return;
		}
	}
	lm32_irq_ack(IRQ_AC97CRREQUEST);
}

static void milkymist_ac97_warm_reset(struct snd_ac97 *ac97)
{
	pr_info("%s: Not implemented\n", __func__);
}

static void milkymist_ac97_cold_reset(struct snd_ac97 *ac97)
{
	pr_info("%s: Not implemented\n", __func__);
}

struct snd_ac97_bus_ops soc_ac97_ops = {
	.read		= milkymist_ac97_read,
	.write		= milkymist_ac97_write,
	.warm_reset	= milkymist_ac97_warm_reset,
	.reset		= milkymist_ac97_cold_reset,
};
EXPORT_SYMBOL_GPL(soc_ac97_ops);

static struct snd_soc_dai_driver milkymist_ac97_dai_driver = {
	.ac97_control = 1,
	.playback = {
		.stream_name = "AC97 Playback",
		.channels_min = 2,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_48000,
		.formats = SNDRV_PCM_FMTBIT_S16_BE,
	},
	.capture = {
		.stream_name = "AC97 Capture",
		.channels_min = 2,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_48000,
		.formats = SNDRV_PCM_FMTBIT_S16_BE,
	},
};

static int __devinit milkymist_ac97_probe(struct platform_device *pdev)
{
	return snd_soc_register_dai(&pdev->dev, &milkymist_ac97_dai_driver);
}

static void __devexit milkymist_ac97_remove(struct platform_device *pdev)
{
	snd_soc_unregister_dai(&pdev->dev);
}

static struct platform_driver milkymist_ac97_driver = {
	.probe  = milkymist_ac97_probe,
	.remove = __devexit_p(milkymist_ac97_remove),
	.driver = {
		.name   = "milkymist-ac97",
		.owner  = THIS_MODULE,
	}
};

static int __init milkymist_ac97_init(void)
{
	return platform_driver_register(&milkymist_ac97_driver);
}
module_init(milkymist_ac97_init);

static void __exit milkymist_ac97_exit(void)
{
	platform_driver_unregister(&milkymist_ac97_driver);
}
module_exit(milkymist_ac97_exit);

MODULE_AUTHOR("Sebastien Bourdeauducq");
MODULE_DESCRIPTION("AC97 driver for Milkymist");
MODULE_LICENSE("GPL");

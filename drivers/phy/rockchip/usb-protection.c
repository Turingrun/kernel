#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/pm_runtime.h>
#include <linux/usb/of.h>
#include <linux/usb/otg.h>
#include <linux/usb/role.h>
#include <linux/usb/typec_mux.h>
#include <linux/wakelock.h>
#include <linux/timer.h>

static struct gpio_desc *irq_gpio;
static struct regulator *reg;
static struct timer_list timer;
static struct work_struct work;

static irqreturn_t protection_irq(int irq, void *data)
{
        mod_timer(&timer, jiffies + msecs_to_jiffies(10));
	return IRQ_HANDLED;
}

static void protection_timer_func(struct timer_list *t)
{
	schedule_work(&work);
}


static void protection_work_func(struct work_struct *w)
{
	int ret;
        int value = 0;
        value = gpiod_get_value_cansleep(irq_gpio);
       if(value == 0)
        {
                if(regulator_is_enabled(reg) > 0)
                {
                      ret = regulator_force_disable(reg);
                }
        }
}

static int protection_probe(struct platform_device *pdev)
{
	int ret;
	int irq;
	struct device_node *node;

	node = pdev->dev.of_node;

	reg = devm_regulator_get_optional(&pdev->dev , "pro");

	if(IS_ERR(reg))	{
		ret = PTR_ERR(reg);
		dev_err(&pdev->dev , "failed to get pro: %d\n" ,ret);
		goto get_err;
	}

	irq_gpio = devm_gpiod_get_optional(&pdev->dev,"irq", GPIOD_IN);
	gpiod_direction_input(irq_gpio);

	irq = of_irq_get(node , 0);
	if(irq > 0)
	{
		ret = devm_request_threaded_irq(&pdev->dev ,irq ,NULL ,protection_irq ,IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING|IRQF_ONESHOT, "protection-irq", NULL);
		if (ret)
                        dev_err(&pdev->dev,
                                "failed to request protection irq handle %d\n",ret);
	}
	INIT_WORK(&work, protection_work_func);

	timer_setup(&timer , protection_timer_func , 0);
	
	return 0;
get_err:
	return ret;
}


static int protection_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id protection_of_match[] = {
    { .compatible = "usb-protection"},
};

static struct platform_driver protection_driver = {
    .driver = {
        .name = "usb-pro",
        .of_match_table = protection_of_match,
    },
    .probe = protection_probe,
    .remove = protection_remove,
};

static int __init usb_protection_init(void)
{
	
	return platform_driver_register(&protection_driver);
}

static void __exit usb_protection_exit(void)
{
	platform_driver_unregister(&protection_driver);

}

module_init(usb_protection_init);
module_exit(usb_protection_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("xxxxx");

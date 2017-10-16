#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/timer.h>


MODULE_LICENSE("GPL");


struct timer_list  my_timer;

void timer_func(unsigned long data)
{
	printk("timer_func jiffies:%ld\n", jiffies);
	printk("time alarm.\n");

	mod_timer(&my_timer, jiffies + 1*HZ / 250);
}

static int __init demon_init(void)
{
	printk("%s,%d\n", __func__, __LINE__);
	printk("HZ:%d\n",HZ);

	my_timer.expires = jiffies + 1*HZ / 250;
	my_timer.function = timer_func;
	init_timer(&my_timer);

	add_timer(&my_timer);
	printk("jiffies:%ld\n", jiffies);

	return 0;
}

static void __exit demon_exit(void)
{
	printk("%s,%d\n", __func__, __LINE__);
	del_timer(&my_timer);
}

module_init(demon_init);
module_exit(demon_exit);

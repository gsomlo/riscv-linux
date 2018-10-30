/*
 * Lowrisc Dummy Keyboard Controller Driver - Translates UART input to fake key events
 * http://www.lowrisc.org/
 *
 * based on opencores Javier Herrero <jherrero@hvsistemas.es>
 * Copyright 2007-2009 HV Sistemas S.L.
 *
 * Licensed under the GPL-2 or later.
 *
 * This driver acts as a shadow console, forwarding serial port events to the main console
 * using the input event mechanism. It's primary use is to allow LowRISC to be used via
 * a serial port without screen and keyboard being connected.
 *
 * It also may be used as a conduit to shadow an approximation of console output to the serial port
 *
 * Since console output is already processed according to the type of device at this point, it would
 * be too complicated to replicate all the behaviour of a serial console. So everything other than
 * emergency hacking should be done via ssh (for editing) or sftp (for uploading)
 */

#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/input-polldev.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <linux/uaccess.h>
#include <asm/sbi.h>

#define DRIVER_NAME "lowrisc-fake"

static struct lowrisc_fake {
  struct platform_device *pdev;
  struct resource *fake;
  spinlock_t lock;
  volatile uint32_t *fake_base;
  struct input_dev *input;
  unsigned short keycodes[128];
} lowrisc_fake_static;

void lowrisc_shadow_console_putchar(int ch)
{
  volatile uint64_t *tx = (volatile uint64_t *)(lowrisc_fake_static.fake_base);
  if (tx)
    {
      *tx = ch & 0x7f; /* just send the US ASCII subset */
    }
  else /* probe function not called yet */
    {
      sbi_console_putchar(ch);
    }
}

static int upper(int ch)
{
  if ( ch >= 'A' && ch <= 'Z' ) return 1;
  switch ( ch )
    {
    case '!' : return 1;
    case '"' : return 1;
    case '#' : return 1;
    case '$' : return 1;
    case '%' : return 1;
    case '^' : return 1;
    case '&' : return 1;
    case '*' : return 1;
    case '<' : return 1;
    case '>' : return 1;
    case '~' : return 1;
    case '@' : return 1;
    case '+' : return 1;
    case '_' : return 1;
    case '(' : return 1;
    case ')' : return 1;
    case '{' : return 1;
    case '}' : return 1;
    case '|' : return 1;
    case ':' : return 1;
    case '?' : return 1;
    default  : return 0;
    }
}

static void lowrisc_keys_poll(struct input_polled_dev *dev)
{
  struct lowrisc_fake *lowrisc_fake = dev->private;
  struct input_dev *input = dev->input;
  unsigned char c;
  volatile uint64_t *rx = (volatile uint64_t *)(lowrisc_fake->fake_base);
  int ch = *rx;
  if (0x200 & ~ch)
    {
      enum {lctrl=0x1d, lshift=0x2a};
      int ctrl;
      rx[0x200] = 0; // pop FIFO
      ch = *rx & 0x7f;
      ctrl = ch >= 1 && ch <= 26;
      if (ctrl)
	{
	  input_report_key(input, lctrl, 1);
          ch = ch + 'a' - 1; // Convert ctrl-key back to normal key
	}
      switch(ch)
	{
	case '\e' : c =  	  0x01; break;
	case '1' : case '!' : c = 0x02; break;
	case '2' : case '@' : c =  	  0x03; break;
	case '3' : case '#' : c =  	  0x04; break;
	case '4' : case '$' : c =  	  0x05; break;
	case '5' : case '%' : c =  	  0x06; break;
	case '6' : case '^' : c =  	  0x07; break;
	case '7' : case '&' : c =  	  0x08; break;
	case '8' : case '*' : c =  	  0x09; break;
	case '9' : case '(' : c =  	  0x0a; break;
	case '0' : case ')' : c =  	      0x0b; break;
	case '-' : case '_' : c =  	  0x0c; break;
	case '=' : case '+' : c =  	  0x0d; break;
	case 0x7f : c = 0x0e; break;
	case '\t' : c = 0x0f; break;
	case 'Q' : case 'q' : c =  	  0x10; break;
	case 'W' : case 'w' : c =  	  0x11; break;
	case 'E' : case 'e' : c =  	  0x12; break;
	case 'R' : case 'r' : c =  	  0x13; break;
	case 'T' : case 't' : c =  	  0x14; break;
	case 'Y' : case 'y' : c =  	  0x15; break;
	case 'U' : case 'u' : c =  	  0x16; break;
	case 'I' : case 'i' : c =  	  0x17; break;
	case 'O' : case 'o' : c =  	  0x18; break;
	case 'P' : case 'p' : c =  	  0x19; break;
	case '[' : case '{' : c =  	  0x1a; break;
	case ']' : case '}' : c =  	  0x1b; break;
	case '\r' : c =  	  0x1c; break;
	case 'A' : case 'a' : c =  	  0x1e; break;
	case 'S' : case 's' : c =  	  0x1f; break;
	case 'D' : case 'd' : c =  	  0x20; break;
	case 'F' : case 'f' : c =  	  0x21; break;
	case 'G' : case 'g' : c =  	  0x22; break;
	case 'H' : case 'h' : c =  	  0x23; break;
	case 'J' : case 'j' : c =  	  0x24; break;
	case 'K' : case 'k' : c =  	  0x25; break;
	case 'L' : case 'l' : c =  	  0x26; break;
	case ';' : case ':' : c =  	  0x27; break;
	case '\'' : case '"' : c =  	  0x28; break;
	case '`' : case '~' : c =  	  0x29; break;
	case '\\' : case '|' : c =  	  0x2b; break;
	case 'Z' : case 'z' : c =   0x2c; break;
	case 'X' : case 'x' : c =   0x2d; break;
	case 'C' : case 'c' : c =   0x2e; break;
	case 'V' : case 'v' : c =   0x2f; break;
	case 'B' : case 'b' : c =   0x30; break;
	case 'N' : case 'n' : c =   0x31; break;
	case 'M' : case 'm' : c =   0x32; break;
	case ',' : case '<' : c =   0x33; break;
	case '.' : case '>' : c =   0x34; break;
	case '/' : case '?' : c =   0x35; break;
	case ' ' : c =   0x39; break;
	default: c = 0; printk("input_report_key 0x%x not translated\n", ch);
	}
      if (upper(ch))
	{
	  input_report_key(input, lshift, 1);
	}
      input_report_key(input, c, 1);
      input_report_key(input, c, 0);
      if (ctrl)
	{
	  input_report_key(input, lctrl, 0);
	}
      if (upper(ch))
	{
	  input_report_key(input, lshift, 0);
	}
      input_sync(input);
    }
}

static int lowrisc_fake_probe(struct platform_device *pdev)
{
  struct input_dev *input;
  struct lowrisc_fake *lowrisc_fake;
  int i, error;
  struct input_polled_dev *poll_dev;
  struct device *dev = &pdev->dev;

  printk("lowrisc_fake_probe\n");
  lowrisc_fake = &lowrisc_fake_static;

  if (lowrisc_fake->fake_base) /* Only one instance allowed */
    {
    return -ENOMEM;
    }

  lowrisc_fake->fake = platform_get_resource(pdev, IORESOURCE_MEM, 0);
  if (!request_mem_region(lowrisc_fake->fake->start, resource_size(lowrisc_fake->fake), "lowrisc_fake"))
    {
    dev_err(&pdev->dev, "cannot request LowRISC UART region\n");
    return -EBUSY;
    }

  lowrisc_fake->fake_base = (volatile uint32_t *)ioremap(lowrisc_fake->fake->start, resource_size(lowrisc_fake->fake));
  printk("fake_keyboard address %llx, remapped to %lx\n", lowrisc_fake->fake->start, (size_t)lowrisc_fake->fake_base);

  poll_dev = devm_input_allocate_polled_device(dev);
  if (!poll_dev) {
    dev_err(dev, "failed to allocate input device\n");
    return -ENOMEM;
  }

  poll_dev->poll_interval = 100;

  poll_dev->poll = lowrisc_keys_poll;
  poll_dev->private = lowrisc_fake;

  input = poll_dev->input;

  lowrisc_fake->input = input;

  input->name = pdev->name;
  input->phys = "lowrisc-fake/input0";

  input->id.bustype = BUS_HOST;
  input->id.vendor = 0x0001;
  input->id.product = 0x0001;
  input->id.version = 0x0100;

  input->keycode = lowrisc_fake->keycodes;
  input->keycodesize = sizeof(lowrisc_fake->keycodes[0]);
  input->keycodemax = ARRAY_SIZE(lowrisc_fake->keycodes);

  __set_bit(EV_KEY, input->evbit);

  for (i = 0; i < ARRAY_SIZE(lowrisc_fake->keycodes); i++) {
    /*
     * Lowrisc lowrisc_faketroller happens to have scancodes match
     * our KEY_* definitions.
     */
    lowrisc_fake->keycodes[i] = i;
    __set_bit(lowrisc_fake->keycodes[i], input->keybit);
  }
  __clear_bit(KEY_RESERVED, input->keybit);

  error = input_register_polled_device(poll_dev);
  if (error) {
    dev_err(dev, "Unable to register input device: %d\n", error);
    return error;
  }

  return 0;
}

static const struct of_device_id lowrisc_fake_of_match[] = {
	{ .compatible = DRIVER_NAME },
	{ }
};

MODULE_DEVICE_TABLE(of, lowrisc_fake_of_match);

static struct platform_driver lowrisc_fake_device_driver = {
	.probe    = lowrisc_fake_probe,
	.driver   = {
		.name = DRIVER_NAME,
		.of_match_table = lowrisc_fake_of_match,
	},
};
module_platform_driver(lowrisc_fake_device_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jonathan Kimmitt <jonathan@kimmitt.uk>");
MODULE_DESCRIPTION("Dummy Keyboard input events for Lowrisc");

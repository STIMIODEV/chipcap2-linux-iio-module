/* chipcap2.c
 * A Linux kernel IIO i2c driver module for the Amphenol Advanced Sensors /
 * Telaire 14 bit ChipCap 2 humidity and temperature sensors.
 *
 * Version: 1.0.0
 * Author:  Matthew J Wolf
 * Date:    03-FEB-2019
 *
 * This driver only works correctly with the ChipCap 2 devices that do not enter
 * sleep mode after power-on reset (POR). This driver does not have any
 * support for the ChipCap 2 Command Mode. This driver does not configure the
 * device's alarm or analog options.
 *
 * The only thing this driver does id transmit to a ChipCap 2 is the Data Fetch
 * command 0xDF. The driver then receives and formats the responses to the
 * Data Fetch command.
 *
 * This driver does not enable any IIO buffers or triggers.
 *
 * The driver was developed with a ChipCap2 device with the part number
 * of CC2D23-SIP.
 *
 * The part numbers of devices that this driver should support:
 * CC2D23 ChipCap 2, digital, 2%, 3.3v
 * CC2D25 ChipCap 2, digital, 2%, 5v
 * CC2D33 ChipCap 2, digital, 3%, 3.3v
 * CC2D35 ChipCap 2, digital, 3%, 5v
 * Sources for the part numbers:
 * [1] Amphenol Thermometrics, App. Guide, AAS-916-127 Rev. J, pp 39.
 * [2] Amphenol Advanced Sensors, "ChipCap 2 humidity and temperature sensor,"
 *     Datasheet AAS-920-558E, Feb. 2015.
 *
 * Copyright (c) 2019 Matthew J. Wolf
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>

#define CHIPCAP2_DATA_FETCH 0xdf

static int chipcap2_read_raw (struct iio_dev *indio_dev,
                              struct iio_chan_spec const *chan, int *val,
                              int *val2, long mask)
{
   u8 buf[5];
   struct i2c_client **client = iio_priv(indio_dev);
   int ret;

   switch (mask) {

   case IIO_CHAN_INFO_RAW:

      if (chan->type == IIO_HUMIDITYRELATIVE) {

        ret =  i2c_smbus_read_i2c_block_data(*client,CHIPCAP2_DATA_FETCH,2,buf);
        if (ret != 2 ) {
           return -EIO;
        }

         *val = (int)( (( buf[0] & 0x3F ) << 8 ) + buf[1] );
      }
      else if (chan->type == IIO_TEMP) {

         ret =  i2c_smbus_read_i2c_block_data(*client,CHIPCAP2_DATA_FETCH,4,buf);
         if (ret != 4 ) {
            return -EIO;
         }

         *val = (int)(( buf[2] << 6 ) + buf[3] / 4 );
      }
      return IIO_VAL_INT;
   case IIO_CHAN_INFO_SCALE:
      if (chan->type == IIO_HUMIDITYRELATIVE) {
         *val = 100;
         *val2 = 16384; // 163.84 * 100
      }
      if (chan->type == IIO_TEMP) {
         *val = 100;
         *val2 = 9929;  // 99.29 * 100
      }
      return IIO_VAL_FRACTIONAL;
   case IIO_CHAN_INFO_OFFSET:
      if (chan->type == IIO_TEMP) {
         *val = -40;
      }
      return IIO_VAL_INT;
   default:
      break;
   }
   return -EINVAL;
}

static const struct iio_chan_spec chipcap2_channels[] = {
   {
      .type = IIO_HUMIDITYRELATIVE,
      .info_mask_separate = BIT(IIO_CHAN_INFO_RAW) |
                            BIT(IIO_CHAN_INFO_SCALE),
   },
   {
      .type = IIO_TEMP,
      .info_mask_separate = BIT(IIO_CHAN_INFO_RAW) |
                            BIT(IIO_CHAN_INFO_SCALE) |
                            BIT(IIO_CHAN_INFO_OFFSET),
   }
};

static const struct iio_info chipcap2_info = {
   .read_raw = chipcap2_read_raw,
   .driver_module = THIS_MODULE,
};

static int chipcap2_probe(struct i2c_client *client,
                          const struct i2c_device_id *id)
{
   struct iio_dev *indio_dev;
   struct i2c_client **data;

   if (!i2c_check_functionality(client->adapter,
                                I2C_FUNC_SMBUS_WRITE_BYTE |
                                I2C_FUNC_SMBUS_READ_WORD_DATA))
      return -EOPNOTSUPP;

   indio_dev = devm_iio_device_alloc(&client->dev, sizeof( *data ));
   if (!indio_dev)
      return -ENOMEM;

   data = iio_priv(indio_dev);
   *data = client;

   i2c_set_clientdata(client, indio_dev);

   indio_dev->dev.parent = &client->dev;
   indio_dev->name = client->name;
   indio_dev->modes = INDIO_DIRECT_MODE;
   indio_dev->info = &chipcap2_info;
   indio_dev->channels = chipcap2_channels;
   indio_dev->num_channels = ARRAY_SIZE(chipcap2_channels);

   return devm_iio_device_register(&client->dev, indio_dev);
}

static int chipcap2_remove(struct i2c_client *client)
{
   struct iio_dev *indio_dev = i2c_get_clientdata(client);

   devm_iio_device_unregister(&client->dev,indio_dev);

   return 0;
}

static const struct i2c_device_id chipcap2_id_table[] = {
   { "cc2d23", 0 },
   { "cc2d25", 1 },
   { "cc2d33", 2 },
   { "cc2d35", 3 },
   {},
};
MODULE_DEVICE_TABLE(i2c, chipcap2_id_table);

static const struct of_device_id chipcap2_of_match[] = {
   {.compatible = "amp,cc2d23" },
   {.compatible = "amp,cc2d25" },
   {.compatible = "amp,cc2d33" },
   {.compatible = "amp,cc2d35" },
   {}
};
MODULE_DEVICE_TABLE(of, chipcap2_of_match);

static struct i2c_driver chipcap2_driver = {
   .driver = {
      .name = "chipcap2",
      .of_match_table = chipcap2_of_match,
   },
   .probe = chipcap2_probe,
   .remove = chipcap2_remove,
   .id_table = chipcap2_id_table,
};

module_i2c_driver(chipcap2_driver);
MODULE_AUTHOR("Matthew J Wolf <matthew.wolf.hpsdr@speciosus.net>");
MODULE_DESCRIPTION("Amphenol ChipCap 2 Humidity and Temperature Sensor (No chip Alarm or Sleep, No IIO Buffers or Triggers)");
MODULE_LICENSE("GPL v2");

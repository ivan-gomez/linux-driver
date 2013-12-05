  /*
   * USB Skeleton driver - 2.2
   *
   * Copyright (C) 2001-2004 Greg Kroah-Hartman (greg@kroah.com)
   *
   *      This program is free software; you can redistribute it and/or
   *      modify it under the terms of the GNU General Public License as
   *      published by the Free Software Foundation, version 2.
   *
   * This driver is based on the 2.6.3 version of drivers/usb/usb-skeleton.c
   * but has been rewritten to be easier to read and use.
   *
   */
  
  #include <linux/kernel.h>
  #include <linux/errno.h>
  #include <linux/init.h>
  #include <linux/slab.h>
  #include <linux/module.h>
  #include <linux/kref.h>
  #include <linux/uaccess.h>
  #include <linux/usb.h>
  #include <linux/mutex.h>
  
  
  /* Define these values to match your devices */
  #define USB_SKEL_VENDOR_ID      0x046d
  #define USB_SKEL_PRODUCT_ID     0xc21a
  
  /* table of devices that work with this driver */
  static const struct usb_device_id skel_table[] = {
          { USB_DEVICE(USB_SKEL_VENDOR_ID, USB_SKEL_PRODUCT_ID) },
          { }                                     /* Terminating entry */
  };
  MODULE_DEVICE_TABLE(usb, skel_table);

  int b_interval;  
  
  /* Get a minor range for your devices from the usb maintainer */
  #define USB_SKEL_MINOR_BASE     192
  

  /* our private defines. if this grows any larger, use your own .h file */
  #define MAX_TRANSFER            (PAGE_SIZE - 512)
  /* MAX_TRANSFER is chosen so that the VM is not stressed by
     allocations > PAGE_SIZE and the number of packets in a page
     is an integer 512 is the largest possible packet on EHCI */
  #define WRITES_IN_FLIGHT        8
  /* arbitrarily chosen */
  
  /* Structure to hold all of our device specific stuff */
  struct usb_skel {
          struct usb_device       *udev;                  /* the usb device for this device */
          struct usb_interface    *interface;             /* the interface for this device */
          struct semaphore        limit_sem;              /* limiting the number of writes in progress */
          struct usb_anchor       submitted;              /* in case we need to retract our submissions */
          struct urb              *int_in_urb;            /* the urb to read data with */
          unsigned char           *int_in_buffer;         /* the buffer to receive data */
          size_t                  int_in_size;           /* the size of the receive buffer */
          size_t                  int_in_filled;         /* number of bytes in the buffer */
          size_t                  int_in_copied;         /* already copied to user space */
          __u8                    int_in_endpointAddr;   /* the address of the int in endpoint */
          int                     errors;                 /* the last request tanked */
          bool                    ongoing_read;           /* a read is going on */
          spinlock_t              err_lock;               /* lock for errors */
          struct kref             kref;
          struct mutex            io_mutex;               /* synchronize I/O with disconnect */
          wait_queue_head_t       int_in_wait;           /* to wait for an ongoing read */
  };
  #define to_skel_dev(d) container_of(d, struct usb_skel, kref)
  
  static struct usb_driver skel_driver;
  static void skel_draw_down(struct usb_skel *dev);
  
  static void skel_delete(struct kref *kref)
  {
          struct usb_skel *dev = to_skel_dev(kref);
  
          usb_free_urb(dev->int_in_urb);
          usb_put_dev(dev->udev);
          kfree(dev->int_in_buffer);
          kfree(dev);
  }
  
  static int skel_open(struct inode *inode, struct file *file)
  {
          struct usb_skel *dev;
          struct usb_interface *interface;
          int subminor;
          int retval = 0;
  
          subminor = iminor(inode);
  
          interface = usb_find_interface(&skel_driver, subminor);
          if (!interface) {
                  pr_err("%s - error, can't find device for minor %d\n",
                          __func__, subminor);
                  retval = -ENODEV;
                  goto exit;
          }
 
         dev = usb_get_intfdata(interface);
         if (!dev) {
                 retval = -ENODEV;
                 goto exit;
         }
 
         retval = usb_autopm_get_interface(interface);
         if (retval)
                 goto exit;
 
         /* increment our usage count for the device */
         kref_get(&dev->kref);
 
         /* save our object in the file's private structure */
         file->private_data = dev;
 
 exit:
         return retval;
 }
 
 static int skel_release(struct inode *inode, struct file *file)
 {
         struct usb_skel *dev;
 
         dev = file->private_data;
         if (dev == NULL)
                 return -ENODEV;
 
         /* allow the device to be autosuspended */
         mutex_lock(&dev->io_mutex);
         if (dev->interface)
                 usb_autopm_put_interface(dev->interface);
         mutex_unlock(&dev->io_mutex);
 
         /* decrement the count on our device */
         kref_put(&dev->kref, skel_delete);
         return 0;
 }
 
 static int skel_flush(struct file *file, fl_owner_t id)
 {
         struct usb_skel *dev;
         int res;
 
         dev = file->private_data;
         if (dev == NULL)
                 return -ENODEV;
 
         /* wait for io to stop */
         mutex_lock(&dev->io_mutex);
         skel_draw_down(dev);
 
         /* read out errors, leave subsequent opens a clean slate */
         spin_lock_irq(&dev->err_lock);
         res = dev->errors ? (dev->errors == -EPIPE ? -EPIPE : -EIO) : 0;
         dev->errors = 0;
         spin_unlock_irq(&dev->err_lock);
 
         mutex_unlock(&dev->io_mutex);
 
         return res;
 }
 
 static void skel_read_int_callback(struct urb *urb)
 {
         struct usb_skel *dev;
 
         dev = urb->context;
 
         spin_lock(&dev->err_lock);
         /* sync/async unlink faults aren't errors */
         if (urb->status) {
                 if (!(urb->status == -ENOENT ||
                     urb->status == -ECONNRESET ||
                     urb->status == -ESHUTDOWN))
                         dev_err(&dev->interface->dev,
                                 "%s - nonzero write int status received: %d\n",
                                 __func__, urb->status);
 
                 dev->errors = urb->status;
         } else {
                 dev->int_in_filled = urb->actual_length;
         }
         dev->ongoing_read = 0;
         spin_unlock(&dev->err_lock);
 
         wake_up_interruptible(&dev->int_in_wait);
 }
 
 static int skel_do_read_io(struct usb_skel *dev, size_t count)
 {
         int rv;
 
         /* prepare a read */
         usb_fill_int_urb(dev->int_in_urb,
                         dev->udev,
                         usb_rcvintpipe(dev->udev,
                                 dev->int_in_endpointAddr),
                         dev->int_in_buffer,
                         min(dev->int_in_size, count),
                         skel_read_int_callback,
                         dev, b_interval);
         /* tell everybody to leave the URB alone */
         spin_lock_irq(&dev->err_lock);
         dev->ongoing_read = 1;
         spin_unlock_irq(&dev->err_lock);
 
         /* submit int in urb, which means no data to deliver */
         dev->int_in_filled = 0;
         dev->int_in_copied = 0;
 
         /* do it */
         rv = usb_submit_urb(dev->int_in_urb, GFP_KERNEL);
         if (rv < 0) {
                 dev_err(&dev->interface->dev,
                         "%s - failed submitting read urb, error %d\n",
                         __func__, rv);
                 rv = (rv == -ENOMEM) ? rv : -EIO;
                 spin_lock_irq(&dev->err_lock);
                 dev->ongoing_read = 0;
                 spin_unlock_irq(&dev->err_lock);
         }
 
         return rv;
 }
 
 static ssize_t skel_read(struct file *file, char *buffer, size_t count,
                          loff_t *ppos)
 {
         struct usb_skel *dev;
         int rv;
         bool ongoing_io;
 
         dev = file->private_data;
 
         /* if we cannot read at all, return EOF */
         if (!dev->int_in_urb || !count)
                 return 0;
 
         /* no concurrent readers */
         rv = mutex_lock_interruptible(&dev->io_mutex);
         if (rv < 0)
                 return rv;
 
         if (!dev->interface) {          /* disconnect() was called */
                 rv = -ENODEV;
                 goto exit;
         }
 
         /* if IO is under way, we must not touch things */
 retry:
         spin_lock_irq(&dev->err_lock);
         ongoing_io = dev->ongoing_read;
         spin_unlock_irq(&dev->err_lock);
 
         if (ongoing_io) {
                 /* nonblocking IO shall not wait */
                 if (file->f_flags & O_NONBLOCK) {
                         rv = -EAGAIN;
                         goto exit;
                 }
                 /*
                  * IO may take forever
                  * hence wait in an interruptible state
                  */
                 rv = wait_event_interruptible(dev->int_in_wait, (!dev->ongoing_read));
                 if (rv < 0)
                         goto exit;
         }
 
         /* errors must be reported */
         rv = dev->errors;
         if (rv < 0) {
                 /* any error is reported once */
                 dev->errors = 0;
                 /* to preserve notifications about reset */
                 rv = (rv == -EPIPE) ? rv : -EIO;
                 /* report it */
                 goto exit;
         }
 
         /*
          * if the buffer is filled we may satisfy the read
          * else we need to start IO
          */
 
         if (dev->int_in_filled) {
                 /* we had read data */
                 size_t available = dev->int_in_filled - dev->int_in_copied;
                 size_t chunk = min(available, count);
 
                 if (!available) {
                         /*
                          * all data has been used
                          * actual IO needs to be done
                          */
                         rv = skel_do_read_io(dev, count);
                         if (rv < 0)
                                 goto exit;
                         else
                                 goto retry;
                 }
                 /*
                  * data is available
                  * chunk tells us how much shall be copied
                  */
		
 
                 if (copy_to_user(buffer,
                                  dev->int_in_buffer + dev->int_in_copied,
                                  chunk))
                         rv = -EFAULT;
                 else
                         rv = chunk;
 
                 dev->int_in_copied += chunk;
                 pr_info("Bytes escritos: %i\n", chunk);
 		 pr_info("Buffer de lectura: %x %x %x %x\n", *buffer, *(buffer+1),*(buffer+2), *(buffer+3));
                 /*
                  * if we are asked for more than we have,
                  * we start IO but don't wait
                  */
                 if (available < count)
                         skel_do_read_io(dev, count - chunk);
         } else {
                 /* no data in the buffer */
                 rv = skel_do_read_io(dev, count);
                 if (rv < 0)
                         goto exit;
                 else
                         goto retry;
         }
 exit:
         mutex_unlock(&dev->io_mutex);
         return rv;
 }
 
 static const struct file_operations skel_fops = {
         .owner =        THIS_MODULE,
         .read =         skel_read,
         .open =         skel_open,
         .release =      skel_release,
         .flush =        skel_flush,
         .llseek =       noop_llseek,
 };
 
 /*
  * usb class driver info in order to get a minor number from the usb core,
  * and to have the device registered with the driver core
  */
 static struct usb_class_driver skel_class = {
         .name =         "js%d",
         .fops =         &skel_fops,
         .minor_base =   USB_SKEL_MINOR_BASE,
 };
 
 static int skel_probe(struct usb_interface *interface,
                       const struct usb_device_id *id)
 {
         struct usb_skel *dev;
         struct usb_host_interface *iface_desc;
         struct usb_endpoint_descriptor *endpoint;
         size_t buffer_size;
         int i;
         int retval = -ENOMEM;

	pr_info("USB control de Lasa y Robert");
 
         /* allocate memory for our device state and initialize it */
         dev = kzalloc(sizeof(*dev), GFP_KERNEL);
         if (!dev) {
                 dev_err(&interface->dev, "Out of memory\n");
                 goto error;
         }
         kref_init(&dev->kref);
         sema_init(&dev->limit_sem, WRITES_IN_FLIGHT);
         mutex_init(&dev->io_mutex);
         spin_lock_init(&dev->err_lock);
         init_usb_anchor(&dev->submitted);
         init_waitqueue_head(&dev->int_in_wait);
 
         dev->udev = usb_get_dev(interface_to_usbdev(interface));
         dev->interface = interface;
         /* set up the endpoint information */
         /* use only the first int-in and int-out endpoints */
         iface_desc = interface->cur_altsetting;
         for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i) {
                 endpoint = &iface_desc->endpoint[i].desc;
 
                 if (!dev->int_in_endpointAddr &&
                     usb_endpoint_is_int_in(endpoint)) {
                         /* we found a int in endpoint */
                         buffer_size = usb_endpoint_maxp(endpoint);
                         dev->int_in_size = buffer_size;
                         dev->int_in_endpointAddr = endpoint->bEndpointAddress;
                         dev->int_in_buffer = kmalloc(buffer_size, GFP_KERNEL);
                         if (!dev->int_in_buffer) {
                                 dev_err(&interface->dev,
                                         "Could not allocate int_in_buffer\n");
                                 goto error;
                         }
                         dev->int_in_urb = usb_alloc_urb(0, GFP_KERNEL);
                         if (!dev->int_in_urb) {
                                 dev_err(&interface->dev,
                                         "Could not allocate int_in_urb\n");
                                 goto error;
                         }
                 }
 
/*                 if (!dev->int_out_endpointAddr &&
                     usb_endpoint_is_int_out(endpoint)) {
                         // we found a int out endpoint 
                         dev->int_out_endpointAddr = endpoint->bEndpointAddress;
                 }
 */        }
         if (!(dev->int_in_endpointAddr)) {
                 dev_err(&interface->dev,
                         "Could not find int-in endpoint\n");
                 pr_info("Could not find int-in endpoint");
                 goto error;
         }
 
         /* save our data pointer in this interface device */
         usb_set_intfdata(interface, dev);
 
         /* we can register the device now, as it is ready */
         retval = usb_register_dev(interface, &skel_class);
         if (retval) {
                 /* something prevented us from registering this driver */
                 dev_err(&interface->dev,
                         "Not able to get a minor for this device.\n");
                 pr_info("Not able to get a minor for this device.");
                 usb_set_intfdata(interface, NULL);
                 goto error;
         }

 	 b_interval = endpoint->bInterval;

         pr_info("Lasa BINTERVAL %x\n", b_interval); 

           
         /* let the user and messages on the system know what node this device is now attached to */
         dev_info(&interface->dev,
                  "USB Skeleton device now attached to USBSkel-%d",
                  interface->minor);
         pr_info("USB Skeleton device now attached to USBSkel-%d",
                  interface->minor);
         return 0;

 error:
         if (dev)
                 /* this frees allocated memory */
                 kref_put(&dev->kref, skel_delete);
         return retval;
 }
 
 static void skel_disconnect(struct usb_interface *interface)
 {
         struct usb_skel *dev;
         int minor = interface->minor;
 
         dev = usb_get_intfdata(interface);
         usb_set_intfdata(interface, NULL);
 
         /* give back our minor */
         usb_deregister_dev(interface, &skel_class);
 
         /* prevent more I/O from starting */
         mutex_lock(&dev->io_mutex);
         dev->interface = NULL;
         mutex_unlock(&dev->io_mutex);
 
         usb_kill_anchored_urbs(&dev->submitted);
 
         /* decrement our usage count */
         kref_put(&dev->kref, skel_delete);
 
         dev_info(&interface->dev, "USB Skeleton #%d now disconnected", minor);
 }
 
 static void skel_draw_down(struct usb_skel *dev)
 {
         int time;
 
         time = usb_wait_anchor_empty_timeout(&dev->submitted, 1000);
         if (!time)
                 usb_kill_anchored_urbs(&dev->submitted);
         usb_kill_urb(dev->int_in_urb);
 }
 
 static int skel_suspend(struct usb_interface *intf, pm_message_t message)
 {
         struct usb_skel *dev = usb_get_intfdata(intf);
 
         if (!dev)
                 return 0;
         skel_draw_down(dev);
         return 0;
 }
 
 static int skel_resume(struct usb_interface *intf)
 {
         return 0;
 }
 
 static int skel_pre_reset(struct usb_interface *intf)
 {
         struct usb_skel *dev = usb_get_intfdata(intf);
 
         mutex_lock(&dev->io_mutex);
         skel_draw_down(dev);
 
         return 0;
 }
 
 static int skel_post_reset(struct usb_interface *intf)
 {
         struct usb_skel *dev = usb_get_intfdata(intf);
 
         /* we are sure no URBs are active - no locking needed */
         dev->errors = -EPIPE;
         mutex_unlock(&dev->io_mutex);
 
         return 0;
 }
 
 static struct usb_driver skel_driver = {
         .name =         "skeleton",
         .probe =        skel_probe,
         .disconnect =   skel_disconnect,
         .suspend =      skel_suspend,
         .resume =       skel_resume,
         .pre_reset =    skel_pre_reset,
         .post_reset =   skel_post_reset,
         .id_table =     skel_table,
         .supports_autosuspend = 1,
 };
 
 module_usb_driver(skel_driver);
 
MODULE_LICENSE("GPL");

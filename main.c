#include<linux/module.h>
#include<linux/platform_device.h>
#include<linux/fs.h>
#include<linux/kernel.h>
#include<linux/device.h>
#include<linux/uaccess.h>
#include<linux/cdev.h>
#include<linux/of.h>
#include<linux/gpio.h>
#include<linux/interrupt.h>


struct interuptDriverData{
    dev_t deviceNumber;
    int interrupt_number;
    uint64_t interrupt_count;
    struct cdev cdev;
    struct class *myclass;
    struct device *mydevice;
};

irqreturn_t gpio_interrupt_handler(int irq,void *dev_id){
    struct interuptDriverData *private_data = dev_id;
    private_data->interrupt_count++;
    return IRQ_HANDLED;
}

static int gpio_interrupt_open(struct inode *inode, struct file *filp){

    struct interuptDriverData *private_data = container_of(inode->i_cdev, struct interuptDriverData, cdev);
    filp->private_data = private_data;

    return 0;
}

static ssize_t gpio_interrupt_read(struct file *filp, char __user *buf, size_t count, loff_t *offset) {
    // Retrieve our private structure from the file pointer
    struct interuptDriverData *private_data = filp->private_data;
    char k_buf[32];
    int len;

    len = snprintf(k_buf, sizeof(k_buf), "%llu\n", private_data->interrupt_count);
    if (*offset >= len) return 0;
    if (count > len - *offset) count = len - *offset;
    if (copy_to_user(buf, k_buf + *offset, count) != 0) return -EFAULT;
    
    *offset += count;
    return count;
}

struct file_operations f_ops = {
    .owner = THIS_MODULE,
    .open = gpio_interrupt_open,
    .read = gpio_interrupt_read,
};


int gpio_interrupt_probe(struct platform_device *pdev){

    int ret;
    struct interuptDriverData *private_data;
    struct device *dev = &pdev->dev;

    private_data = devm_kzalloc(dev,sizeof(struct interuptDriverData),GFP_KERNEL);
    if(private_data == NULL){
        return -ENOMEM;
    }

    platform_set_drvdata(pdev,private_data);

    private_data->interrupt_number = platform_get_irq(pdev,0);
    if(private_data->interrupt_number < 0){
        dev_err(dev,"failed to get the irq\n");
        return private_data->interrupt_number;
    }

    ret = devm_request_irq(dev,private_data->interrupt_number,gpio_interrupt_handler,0,"gpio_irq_counter",private_data);
    if(ret < 0){
        dev_err(dev,"irq_handler request failed\n");
        return ret;
    }


    ret = alloc_chrdev_region(&private_data->deviceNumber,0,1,"interruptCountCharDevice");
    if(ret < 0){
        dev_err(dev,"error carring out alloc_chrdev_region\n");
        return ret;
    }

    cdev_init(&private_data->cdev,&f_ops);

    private_data->cdev.owner = THIS_MODULE;

    ret = cdev_add(&private_data->cdev,private_data->deviceNumber,1);
    if(ret < 0){
        dev_err(dev,"Error carring out cdev_add\n");
        goto cdev_failed;
    }

    private_data->myclass = class_create("gpio_interrupt_class");
    if(IS_ERR(private_data->myclass)){
        goto class_create_failed;
    }

    private_data->mydevice = device_create(private_data->myclass,NULL,private_data->deviceNumber,NULL,"gpio_interrupt_counter_driver");
    if(IS_ERR(private_data->mydevice)){
        goto device_create_failed;
    }

    dev_info(dev,"gpio_interrupt_driver probed sucessfully\n");
    return 0;


device_create_failed : 
    class_destroy(private_data->myclass);

class_create_failed : 
    cdev_del(&private_data->cdev);

cdev_failed : 
    unregister_chrdev_region(private_data->deviceNumber,1);
    return ret;
}


int gpio_interrupt_remove(struct platform_device *pdev){

    struct interuptDriverData *private_data = platform_get_drvdata(pdev);

    device_destroy(private_data->myclass,private_data->deviceNumber);
    class_destroy(private_data->myclass);
    cdev_del(&private_data->cdev);
    unregister_chrdev_region(private_data->deviceNumber,1);

    pr_info("gpio_interrupt_device removed sucessfully\n");
    return 0;
}


const struct of_device_id gpio_interrupt_compatiable[] = {

    {.compatible = "vik,testdriver"},
    { }

};

MODULE_DEVICE_TABLE(of, gpio_interrupt_compatiable);

static struct platform_driver gpio_platform_driver = {

    .probe = gpio_interrupt_probe,
    .remove = gpio_interrupt_remove,
    .driver = {
        .name = "vik_gpio_interrupt",
        .of_match_table = gpio_interrupt_compatiable,
    },
};


module_platform_driver(gpio_platform_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("vikram");
MODULE_DESCRIPTION("a simple interrupt driven character driver");

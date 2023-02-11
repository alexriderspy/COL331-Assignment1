#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <linux/timer.h>
#include <linux/signal.h>
#include <linux/printk.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("sreemanti");

#define PROCFS_MAX_SIZE 1024
#define PROCFS_NAME "sig_target"

static struct timer_list my_timer;

struct proc_dir_entry *proc_file;

static char procfs_buffer[1000];
unsigned long current_procfs_buffer_size;

static ssize_t procfile_read(struct file* file,char* buffer, size_t len, loff_t* offset);
static ssize_t procfile_write(struct file* file,const char* buffer,size_t len,loff_t *offset);

static int lens_buffer[100];
static int index;
static struct task_struct *task;

static struct proc_ops file_ops = 
{
	.proc_read=procfile_read,
	.proc_write=procfile_write
};

static ssize_t procfile_read(struct file *file,char *buffer, size_t len, loff_t *offset) {
	int ret;

	printk(KERN_INFO "procfile_read called %s", PROCFS_NAME);
	ret = copy_to_user(buffer, procfs_buffer, current_procfs_buffer_size);
	printk(KERN_INFO "%d\n",ret);
	printk(KERN_INFO "current buffer has %s\n",procfs_buffer);
	return 0;
}

static ssize_t procfile_write(struct file *file,const char *buffer,size_t len,loff_t *offset) {
	char temp_buffer[50];
	unsigned long procfs_buffer_size;

	memset(temp_buffer,0,sizeof(temp_buffer));
	procfs_buffer_size=len;
	printk("length of buf = %ld\n",len);
	printk("abt to be written");
	if (procfs_buffer_size > PROCFS_MAX_SIZE) {
		printk(KERN_INFO "buffer is high");
		procfs_buffer_size = PROCFS_MAX_SIZE;
	}
	if (copy_from_user(temp_buffer, buffer, procfs_buffer_size)) {
		printk(KERN_INFO "cannot copy from user");
		return -EFAULT;
	}
	printk(KERN_INFO "written");
	printk(KERN_INFO "text written = %s\n",temp_buffer);
	memcpy(procfs_buffer+current_procfs_buffer_size, temp_buffer, procfs_buffer_size);
	current_procfs_buffer_size += procfs_buffer_size;
	lens_buffer[index++] = current_procfs_buffer_size;
	printk("current total text = %s\n", procfs_buffer);
	printk("current total length=%ld\n", current_procfs_buffer_size);
	return procfs_buffer_size;
}

static void my_timer_callback(struct timer_list *t)
{
	int len_pid, len_signal, part, num;
	struct kernel_siginfo info;
	long int pid,ret;
	long int signal;
	char buf[50];

	pid = 0;
	signal = 0;
	if(current_procfs_buffer_size!=0) {
		part = 0;
		len_pid = 0;
		num = 0;
		for(int i=0;i<current_procfs_buffer_size;++i) {
			//printk("index = %d\n", i);
			if(procfs_buffer[i] == ',') {
				memset(buf, 0, sizeof(buf));
				memcpy(buf, procfs_buffer+i-len_pid, len_pid);
				ret = kstrtol(buf, 10, &pid);
				printk("pid ret = %ld\n", ret);
				if (ret < 0)
					return;
				printk("pid read = %ld\n", pid);
				++i;
				part=1;
				len_signal=0;
			}
			if(part==0)
				len_pid++;
			if(lens_buffer[num]-1 == i) {
				memset(buf, 0, sizeof(buf));
				printk("eof %d\n",i);
				printk("eof %d\n",len_signal);
				memcpy(buf, procfs_buffer+i-len_signal+1, len_signal);
				printk("eof string is %s\n", buf);
				ret = kstrtol(buf, 10, &signal);
				printk("signal ret = %ld\n", ret);
				if (ret < 0)
					return;
				printk("signal read = %ld\n", signal);
				len_pid = 0;
				part = 0;
				num++;
				task = pid_task(find_vpid(pid),PIDTYPE_PID);
				if (task == NULL)
					printk("pid = %ld does not exist\n", pid);
				else {
					memset(&info, 0, sizeof(struct kernel_siginfo));
					info.si_signo = signal;
					ret = send_sig_info(signal,&info,task);
					printk("ret of sending signal is %ld\n", ret);
					if(ret < 0)
						printk("error sending signal");
				}
			}
			if(part==1)
				len_signal++;
		}
	}
	printk("the buffer is empty\n");
	mod_timer(&my_timer, jiffies+msecs_to_jiffies(1000));
}

static int __init procfsig_init(void) {
	memset(procfs_buffer, 0, sizeof(procfs_buffer));
	current_procfs_buffer_size=0;
	proc_file =  proc_create(PROCFS_NAME, 0666, NULL, &file_ops);
	if (proc_file == NULL) {
		printk(KERN_INFO "Cannot create the file");
		return -ENOMEM;
	}
	printk(KERN_INFO "created");
	index = 0;
	timer_setup(&my_timer, my_timer_callback, 0);
	mod_timer(&my_timer, jiffies+msecs_to_jiffies(1000));
	printk(KERN_INFO "set up timer");
	return 0;
}

static void __exit procfsig_cleanup(void)
{
	proc_remove(proc_file);
	printk(KERN_INFO "removed");
	del_timer(&my_timer);
	printk(KERN_INFO "removed timer");
	return;
}


module_init(procfsig_init);
module_exit(procfsig_cleanup);








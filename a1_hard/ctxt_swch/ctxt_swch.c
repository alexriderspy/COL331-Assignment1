#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/printk.h>

struct pid_node{
	pid_t pid;
	struct list_head next_prev_list;
};

struct list_head mprocs;
LIST_HEAD(mprocs);

SYSCALL_DEFINE1(registr, pid_t, pid) {
	struct task_struct *task;
	struct list_head *ptr;
	struct pid_node *entry;
	int found = 0;
	pid_t tpid;
	struct pid_node *newnode;

	if (pid < 1)
		return -22;
	task = pid_task(find_vpid(pid), PIDTYPE_PID);
	if (task == NULL) {
		printk("pid does not exist\n");
		return -3;
	}
	list_for_each(ptr, &mprocs) {
		entry = list_entry (ptr, struct pid_node, next_prev_list);
		tpid = entry->pid;
		if (tpid == pid)
			found = 1;
	}
	if (found == 1)
		return -4;
	newnode = kmalloc(sizeof(*newnode), GFP_KERNEL);
	newnode->pid = pid;
	list_add_tail(&newnode->next_prev_list, &mprocs);
	printk(KERN_INFO "Added pid = %d", pid);
	return 0;
}

SYSCALL_DEFINE1(fetch, struct pid_ctxt_switch*, stats)
{
	unsigned long inv = 0;
	unsigned long v = 0;
	struct list_head *ptr;
	struct pid_node *entry;
	pid_t pid;
	struct task_struct *task;
	struct pid_ctxt_switch *kstats;
	int r;

	list_for_each(ptr, &mprocs)
	{
		entry = list_entry(ptr, struct pid_node, next_prev_list);
		if (entry == NULL)
			return -22;
		pid = entry->pid;
		task = pid_task(find_vpid(pid), PIDTYPE_PID);
		if (task == NULL)
			return -22;
		inv += task->nivcsw;
		v += task->nvcsw;
	}
	printk(KERN_INFO "counted successfully\n");
	kstats = kmalloc(sizeof(kstats), GFP_KERNEL);
	printk(KERN_INFO "Assigned memory for kstats\n");
	kstats->ninvctxt = inv;
	kstats->nvctxt = v;
	printk(KERN_INFO "kstats has values for ctxts");
	if (kstats == NULL)
		return -22;
	r = copy_to_user(stats, kstats, sizeof(kstats));
	printk(KERN_INFO "copied succesfully ret = %d\n", r);
	printk(KERN_INFO "involuntary context switches = %ld\n", inv);
	printk(KERN_INFO "voluntary context switches = %ld\n", v);

	return 0;
}

SYSCALL_DEFINE1(deregistr, pid_t, pid)
{
	struct list_head *ptr;
	struct list_head *tbdel;
	struct pid_node *entry;
	int found = 0;
	pid_t tpid;

	if (pid < 1)
		return -22;
	list_for_each(ptr, &mprocs) {
		entry = list_entry(ptr, struct pid_node, next_prev_list);
		tpid = entry->pid;
		if (tpid == pid) {
			tbdel = ptr;
			found = 1;
		}
	}
	if (found == 0) {
		return -3;
	}
	list_del(tbdel);
	printk(KERN_INFO "Deleted process with pid = %d\n", pid);
	return 0;
}

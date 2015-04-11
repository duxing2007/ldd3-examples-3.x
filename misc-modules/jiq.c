/*
 * jiq.c -- the just-in-queue module
 *
 * Copyright (C) 2001 Alessandro Rubini and Jonathan Corbet
 * Copyright (C) 2001 O'Reilly & Associates
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form, so long as an
 * acknowledgment appears in derived source files.  The citation
 * should list that the code comes from the book "Linux Device
 * Drivers" by Alessandro Rubini and Jonathan Corbet, published
 * by O'Reilly & Associates.   No warranty is attached;
 * we cannot take responsibility for errors or fitness for use.
 *
 * $Id: jiq.c,v 1.7 2004/09/26 07:02:43 gregkh Exp $
 */
 
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/fs.h>     /* everything... */
#include <linux/proc_fs.h>
#include <linux/errno.h>  /* error codes */
#include <linux/workqueue.h>
#include <linux/preempt.h>
#include <linux/interrupt.h> /* tasklets */
#include <linux/seq_file.h>

MODULE_LICENSE("Dual BSD/GPL");

/*
 * The delay for the delayed workqueue timer file.
 */
static long delay = 1;
module_param(delay, long, 0);


/*
 * This module is a silly one: it only embeds short code fragments
 * that show how enqueued tasks `feel' the environment
 */

#define LIMIT	(PAGE_SIZE-128)	/* don't print any more after this size */

/*
 * Print information about the current environment. This is called from
 * within the task queues. If the limit is reched, awake the reading
 * process.
 */
static DECLARE_WAIT_QUEUE_HEAD (jiq_wait);





/*
 * Keep track of info we need between task queue runs.
 */
static struct clientdata {
	struct seq_file *m;
	unsigned long jiffies;
	long delay;
	int wait_cond;
	struct delayed_work jiq_work;
	struct work_struct work;
} jiq_data;

#define SCHEDULER_QUEUE ((task_queue *) 1)



static void jiq_print_tasklet(unsigned long);
static DECLARE_TASKLET(jiq_tasklet, jiq_print_tasklet, (unsigned long)&jiq_data);


/*
 * Do the printing; return non-zero if the task should be rescheduled.
 */
static int jiq_print(void *ptr)
{
	struct clientdata *data = ptr;
	struct seq_file *m = data->m;
	unsigned long j = jiffies;

	if (m->count > LIMIT) {
		data->wait_cond = 1;
		wake_up_interruptible(&jiq_wait);
		return 0;
	}

	if (m->count == 0)
		seq_puts(m,"    time  delta preempt   pid cpu command\n");

  	/* intr_count is only exported since 1.3.5, but 1.99.4 is needed anyways */
	seq_printf(m, "%9li  %4li     %3i %5i %3i %s\n",
			j, j - data->jiffies,
			preempt_count(), current->pid, smp_processor_id(),
			current->comm);

	data->jiffies = j;
	return 1;
}

#define DEFINE_PROC_SEQ_FILE(_name) \
	static int _name##_proc_open(struct inode *inode, struct file *file)\
	{\
		return single_open(file, _name##_proc_show, NULL);\
	}\
	\
	static const struct file_operations _name##_proc_fops = {\
		.open		= _name##_proc_open,\
		.read		= seq_read,\
		.llseek		= seq_lseek,\
		.release	= single_release,\
	};


/*
 * Call jiq_print from a work queue
 */
static void jiq_print_wq(struct work_struct *work)
{
	struct clientdata *data = container_of(work, struct clientdata, jiq_work.work);

	if (! jiq_print (data))
		return;

	schedule_delayed_work(&jiq_data.jiq_work, data->delay);
}

/*
 * Call jiq_print from a non-delayed work queue
 */
static void jiq_print_work(struct work_struct *work)
{
	struct clientdata *data = container_of(work, struct clientdata, work);

	if (! jiq_print (data))
		return;

	schedule_work(&jiq_data.work);
}

static int jiq_read_wq_proc_show(struct seq_file *m, void *v)
{
	DEFINE_WAIT(wait);
	
	jiq_data.m = m;              /* print in this place */
	jiq_data.jiffies = jiffies;      /* initial time */
	jiq_data.delay = 0;
    
	prepare_to_wait(&jiq_wait, &wait, TASK_INTERRUPTIBLE);
	schedule_work(&jiq_data.work);
	schedule();
	finish_wait(&jiq_wait, &wait);

	return 0;
}

DEFINE_PROC_SEQ_FILE(jiq_read_wq)

static int jiq_read_wq_delayed_proc_show(struct seq_file *m, void *v)
{
	DEFINE_WAIT(wait);
	
	jiq_data.m = m;              /* print in this place */
	jiq_data.jiffies = jiffies;      /* initial time */
	jiq_data.delay = delay;
    
	prepare_to_wait(&jiq_wait, &wait, TASK_INTERRUPTIBLE);
	schedule_delayed_work(&jiq_data.jiq_work, delay);
	schedule();
	finish_wait(&jiq_wait, &wait);

	return 0;
}

DEFINE_PROC_SEQ_FILE(jiq_read_wq_delayed)

/*
 * This one, instead, tests out the timers.
 */

static struct timer_list jiq_timer;

static void jiq_timedout(unsigned long ptr)
{
	jiq_print((void *)ptr);            /* print a line */

	wake_up_interruptible(&jiq_wait);  /* awake the process */
}

static int jiq_read_run_timer_proc_show(struct seq_file *m, void *v)
{

	jiq_data.m = m;
	jiq_data.jiffies = jiffies;
	jiq_data.wait_cond = 0;

	init_timer(&jiq_timer);              /* init the timer structure */
	jiq_timer.function = jiq_timedout;
	jiq_timer.data = (unsigned long)m;
	jiq_timer.expires = jiffies + HZ; /* one second */

	jiq_print(&jiq_data);   /* print and go to sleep */
	add_timer(&jiq_timer);
	wait_event_interruptible(jiq_wait, jiq_data.wait_cond);  /* RACE */
	del_timer_sync(&jiq_timer);  /* in case a signal woke us up */
    
	return 0;
}

DEFINE_PROC_SEQ_FILE(jiq_read_run_timer)

/*
 * Call jiq_print from a tasklet
 */
static void jiq_print_tasklet(unsigned long ptr)
{
	if (jiq_print ((void *) ptr))
		tasklet_schedule (&jiq_tasklet);
}

static int jiq_read_tasklet_proc_show(struct seq_file *m, void *v)
{
	jiq_data.m = m;              /* print in this place */
	jiq_data.jiffies = jiffies;      /* initial time */
	jiq_data.wait_cond = 0;

	tasklet_schedule(&jiq_tasklet);
	wait_event_interruptible(jiq_wait, jiq_data.wait_cond);    /* sleep till completion */

	return 0;
}

DEFINE_PROC_SEQ_FILE(jiq_read_tasklet)


/*
 * the init/clean material
 */

static int jiq_init(void)
{

	/* this line is in jiq_init() */
	INIT_DELAYED_WORK(&jiq_data.jiq_work, jiq_print_wq);
	INIT_WORK(&jiq_data.work, jiq_print_work);

	proc_create("jiqwq", 0, NULL, &jiq_read_wq_proc_fops);
	proc_create("jiqwqdelay", 0, NULL, &jiq_read_wq_delayed_proc_fops);
	proc_create("jitimer", 0, NULL, &jiq_read_run_timer_proc_fops);
	proc_create("jiqtasklet", 0, NULL, &jiq_read_tasklet_proc_fops);

	return 0; /* succeed */
}

static void jiq_cleanup(void)
{
	remove_proc_entry("jiqwq", NULL);
	remove_proc_entry("jiqwqdelay", NULL);
	remove_proc_entry("jitimer", NULL);
	remove_proc_entry("jiqtasklet", NULL);
}


module_init(jiq_init);
module_exit(jiq_cleanup);

/*
* Name    : sys_xconcat.c
* Author  : Lokesh Lagudu
* Date    : 03/01/2014
*
* System Call that concatenate one or more
* files into a destination target file
*/
#include <linux/linkage.h>
#include <linux/moduleloader.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/syscalls.h>


asmlinkage extern long (*sysptr)(void *arg);

/*
* Struture variable which stores the arguments
* passed from the userland program
*/
struct kargs {
        __user const char *outfile;
        __user const char **infiles;
        unsigned int infile_count;
        int oflags;
        mode_t mode;
        unsigned int flags;
};

static int read_file(void *args, int argslen);
static int validate(void *args, int argslen);

asmlinkage long xconcat(void *arg, int len)
{
        ssize_t re;
	re = validate(arg, len);

        if (re == 0)
                re = read_file(arg, len);

        return re;
}

/*
* validate    - Checks for all possible error cases
* @arg:         File structure to validate
* @len:         Length of the file structure
*/
static int validate(void *arg, int len)
{
        ssize_t re;
        int i = 0;
        struct file *wfilp = NULL;
        struct kargs *k_args1;
        k_args1 = kmalloc(sizeof(struct kargs), GFP_KERNEL);

        /* Copy_from_user copies block of data from user space to kernel */
        re = copy_from_user(k_args1, arg, len);
        struct file *fd[k_args1->infile_count];

        /* Memory not allocated in the kernel */
        if (k_args1 == NULL) {
                printk(KERN_DEBUG"\nError: Memory not allocated");
                re = -ENOMEM;
                goto cleanup;
        }

        /* Data not copied from userspace to kernel space */
        if (re < 0) {
                printk(KERN_DEBUG"\nError: Data not copied properly");
                re =  -EFAULT;
                goto cleanup;
        }

        /* Incorrect length of struct variable defined in len argument */
        if (len != (sizeof(struct kargs))) {
		printk(KERN_DEBUG"\nError: Length of the struct varied");
                re = -EFAULT;
                goto cleanup;
        }

        /* invalid flag combination */
        if (k_args1->flags > 14) {
                printk(KERN_DEBUG"\nError: Invalid flags combination");
                re = -EINVAL;
                goto cleanup;
        }

        /* Outfile not defined */
        if (k_args1->outfile == NULL) {
                printk(KERN_DEBUG"\nError: Output file not found");
                re = -EINVAL;
                goto cleanup;
        }

        /* Infile not defined */
        if (k_args1->infile_count < 1) {
                printk(KERN_DEBUG"\nError: Input file not found");
                re = -EINVAL;
                goto cleanup;
        }

        /* Count of infiles exceeds the limit */
        if (k_args1->infile_count > 10) {
                printk(KERN_DEBUG"\nError: Input file limit exceeded");
                re = -EINVAL;
                goto cleanup;
        }

        /* Error opening the outfiele */
        if (k_args1->oflags & O_TRUNC)
                wfilp = filp_open(k_args1->outfile,
                                k_args1->oflags ^ O_TRUNC, k_args1->mode);
        else
                wfilp = filp_open(k_args1->outfile, k_args1->oflags,
                        k_args1->mode);

	if (!(wfilp) || IS_ERR(wfilp)) {
                printk(KERN_DEBUG"\nERROR: Output file unavailable or cannot be opened");
                return -EFAULT;
                goto cleanup;
        }

        /* Error opening the infile */
        for (i = 0; i < k_args1->infile_count; i++) {
                fd[i] = filp_open(k_args1->infiles[i], O_RDONLY, 0);

                if (!fd[i] || IS_ERR(fd[i])) {
                        printk(KERN_DEBUG"\nERROR: Input file unavailabe or cannot be opened");
                        return -EFAULT;
                        goto endfor;
                }

                /* Output file and input file are the same */
                if (fd[i]->f_dentry->d_inode == wfilp->f_dentry->d_inode) {
                        printk(KERN_DEBUG"\nERROR: Output and Input file are the same");
                        return -EFAULT;
                        filp_close(fd[i], NULL);
                        goto cleanup;
                }

endfor:
                filp_close(fd[i], NULL);
        }

        re = 0;

cleanup:
        /* Eliminating memory leaks */
        if ((wfilp) && !IS_ERR(wfilp))
                filp_close(wfilp, NULL);
        kfree(k_args1);
        return re;
}

/*
* read_file   - Performs reading from the input files and
*               writing into the output file
* @arg:         File structure
* @len:         Length of the file structure
*/
static int read_file(void *arg, int len)
{
        ssize_t re = 0;
        struct kargs *k_args1;
        mm_segment_t old_fs;
        int bytes = 0, wbytes = 0;
        int j = 0;
        int fc = 0, bc = 0, brc = 0;
        int pr = 0;
        struct file *wfilp = NULL;
        struct kstat stat;
        void *buf = NULL;
        k_args1 = kmalloc(sizeof(struct kargs), GFP_KERNEL);
        /* Copy data from user space to kernel space */
        re = copy_from_user(k_args1, arg, len);
        struct file *fd[k_args1->infile_count];

/* Variable only for EXTRA_CREDIT code */
#ifdef EXTRA_CREDIT

        struct file *temp = NULL;
        mode_t tmode;
        char *tem = "tempfile.tmp";
        struct dentry *tDentry = NULL, *wDentry = NULL;
        struct inode *tempDir = NULL, *writeDir = NULL;
        int tempRbytes = 0, tempWbytes = 0;

#endif

        if (re < 0) {
                printk(KERN_DEBUG"\nError: Data not copied properly");
                re =  -EFAULT;
                goto cleanup;
        }

        /* Allocating a temporary buffer for reading from
        *  infiles and writing into the output file
        */
	buf = kmalloc(4096, GFP_KERNEL);

        if (buf == NULL) {
                printk(KERN_DEBUG"\nBuffer space not allocated");
                re = -ENOMEM;
                goto cleanup;
        }
        k_args1->outfile = getname((char *)k_args1->outfile);

        if (k_args1->flags >= 2) {

#ifdef EXTRA_CREDIT

                /* Opening the output file without any data corruption
                *  So, checking whether the flags for the output file
                *  has O_TRUNC */
                if ((k_args1->oflags & O_CREAT) == O_CREAT
                        && (k_args1->oflags & O_EXCL) == O_EXCL)
                        k_args1->oflags = k_args1->oflags ^ O_EXCL;

                if (k_args1->oflags & O_TRUNC)
                        wfilp = filp_open(k_args1->outfile, O_RDWR ,
                                k_args1->mode);

                else
                        wfilp = filp_open(k_args1->outfile,
                                k_args1->oflags, k_args1->mode);

                /* Check for errors in opening the file */
                if (!(wfilp) || IS_ERR(wfilp)) {
                        printk(KERN_DEBUG"\nERROR: Cannot open the output file");
                        re = -EFAULT;
                        goto cleanup1;
                }

                /* Check whether the output file has write operations*/
                if (!wfilp->f_op->write) {
                        printk(KERN_DEBUG"\nError: unable to write");
                        re = -EFAULT;
                        if ((k_args1->oflags & O_CREAT) == O_CREAT)
                                vfs_unlink(writeDir, wDentry);
			 goto cleanup1;
                }

                tmode = wfilp->f_dentry->d_inode->i_mode;
                temp = filp_open(tem, O_RDWR | O_CREAT, tmode);

                if (!(temp) || IS_ERR(temp)) {
                        printk(KERN_DEBUG"\nERROR: Cannot open output file");
                        re = -EFAULT;
                        goto cleanup1;
                }

                if (!temp->f_op->write) {
                        printk(KERN_DEBUG"\nERROR: Cannot write the output file");
                        re = -EFAULT;
                        if ((k_args1->oflags & O_CREAT) == O_CREAT)
                                vfs_unlink(writeDir, wDentry);
                        goto cleanup1;
                }

                temp->f_pos = 0;
                old_fs = get_fs();
                set_fs(KERNEL_DS);

                /* Coping the data from the output file into the
                *  temporary file unless and untill O_APPEND flag
                * is specified by the user */
                if (k_args1->oflags & O_APPEND) {
                        do {
                                tempRbytes = vfs_read(wfilp, buf,
                                        4096, &wfilp->f_pos);
                                tempWbytes = vfs_write(temp, buf, tempRbytes,
                                        &temp->f_pos);
                        } while (tempRbytes >= 4096);
                }
                set_fs(old_fs);

                /* Copying the inode and dentry values for the output
                *  and the temporary file */
                tempDir = temp->f_dentry->d_parent->d_inode;
                tDentry = temp->f_dentry;
                writeDir = wfilp->f_dentry->d_parent->d_inode;
		wDentry = wfilp->f_dentry;

                /* Opening the input files and reading the data into the
                *  buffer and the writing the data from the buffer to
                *  the temporary file */
                for (j = 0; j < k_args1->infile_count; j++) {
                        fd[j] = filp_open(k_args1->infiles[j], O_RDONLY, 0);

                        if (!fd[j] || IS_ERR(fd[j])) {
                                printk(KERN_DEBUG"\nERROR: Cannot open input file");
                                re = -EFAULT;
                                filp_close(fd[j], NULL);
                                goto cleanup1;
                        }

                        if (!fd[j]->f_op->read) {
                                printk(KERN_DEBUG"\nERROR: Cannot read input file");
                                re = -EFAULT;
                                filp_close(fd[j], NULL);
                                goto cleanup1;
                        }

                        fd[j]->f_pos = 0;
                        old_fs = get_fs();
                        set_fs(KERNEL_DS);

                        /* Read and write operations performed below. Execute
                        *  the below steps until the complete data from the
                        *  infiles are copied into the temporary file */
                        do {
                                bytes = vfs_read(fd[j], buf, 4096,
                                        &fd[j]->f_pos);
                                if (bytes < 0) {
                                        filp_close(temp, NULL);
                                        vfs_unlink(tempDir, tDentry);
                                        re = bytes;
                                        filp_close(fd[j], NULL);
                                        goto cleanup1;
                                }

                                wbytes = vfs_write(temp, buf, bytes,
					&temp->f_pos);
                                if (wbytes < 0) {
                                        filp_close(temp, NULL);
                                        vfs_unlink(tempDir, tDentry);
                                        re = wbytes;
                                        filp_close(fd[j], NULL);
                                        goto cleanup1;
                                }
                                bc = bc + wbytes;
                        } while (bytes >= 4096);

                        vfs_stat(k_args1->infiles[j], &stat);
                        brc = brc + stat.size;
                        fc = fc + 1;
                        set_fs(old_fs);
                        filp_close(fd[j], NULL);
                }

                /* If read-write is successful, remane the temporary
                *  file to output file and unlink the output file */
                vfs_rename(tempDir, tDentry, writeDir, wDentry);
                vfs_unlink(writeDir, wDentry);

                if (k_args1->flags == 6 || k_args1->flags == 8)
                        re = fc;

                else if (k_args1->flags == 9 || k_args1->flags == 11) {
                        pr = (bc / brc) * 100;
                        re = pr;
                }

                else
                        re = bc;

cleanup1:
                if ((k_args1->oflags & O_CREAT) == O_CREAT)
                        vfs_unlink(writeDir, wDentry);

                if ((temp) && !IS_ERR(temp))
                        filp_close(temp, NULL);

		if ((wfilp) && !IS_ERR(wfilp))
                        filp_close(wfilp, NULL);

#endif
        } else {

                /* Opening the output file without any data corruption
                *  So, checking whether the flags for the output file
                *  has O_TRUNC */
                if ((k_args1->oflags & O_CREAT) == O_CREAT &&
                        (k_args1->oflags & O_EXCL) == O_EXCL)
                        wfilp = filp_open(k_args1->outfile,
                                k_args1->oflags ^ O_EXCL, k_args1->mode);

                else
                        wfilp = filp_open(k_args1->outfile, k_args1->oflags,
                                k_args1->mode);

                if (!(wfilp) || IS_ERR(wfilp)) {
                        printk(KERN_DEBUG"\nERROR: Cannot open the output file");
                        re = -EFAULT;
                        goto cleanup;
                }

                if (!wfilp->f_op->write) {
                        printk(KERN_DEBUG"\nError: unable to write");
                        re = -EFAULT;
                        goto cleanup;
                }

                wfilp->f_pos = 0;

                /* Reading the data from the infiles and writing
                *  the data into the output file
                */
                for (j = 0; j < k_args1->infile_count; j++) {
                        fd[j] = filp_open(k_args1->infiles[j], O_RDONLY, 0);

                        if (!fd[j] || IS_ERR(fd[j])) {
                                printk(KERN_DEBUG"\nERROR: Cannot open the input file");
                                re = -EFAULT;
				goto efor;
                        }
                        if (!fd[j]->f_op->read) {
                                printk(KERN_DEBUG"\nERROR: Cannot read the input file");
                                re = -EFAULT;
                                goto efor;
                        }

                        fd[j]->f_pos = 0;
                        old_fs = get_fs();
                        set_fs(KERNEL_DS);

                        /* Read and write operations performed below. Execute
                        *  the below steps until the complete data from the
                        *  infiles are copied into the temporary file */
                        do {
                                bytes = vfs_read(fd[j], buf, 4096,
                                        &fd[j]->f_pos);

                                if (bytes < 0)
                                        goto efor;

                                wbytes = vfs_write(wfilp, buf, bytes,
                                        &wfilp->f_pos);

                                if (wbytes < 0)
                                        goto efor;

                                bc = bc + wbytes;
                        } while (bytes >= 4096);

                        vfs_stat(k_args1->infiles[j], &stat);
                        brc = brc + stat.size;
                        fc = fc + 1;
                        set_fs(old_fs);
efor:
                        filp_close(fd[j], NULL);
                }

                if (k_args1->flags == 6 || k_args1->flags == 8)
                        re = fc;
		else if (k_args1->flags == 9 || k_args1->flags == 11) {
                        pr = (bc / brc) * 100;
                        re = pr;
                } else
                        re = bc;

cleanup:
                if ((wfilp) && !IS_ERR(wfilp))
                        filp_close(wfilp, NULL);
        }

        putname(k_args1->outfile);
        kfree(buf);
        return re;
}

static int __init init_sys_xconcat(void)
{
        printk(KERN_DEBUG"\ninstalled new sys_xconcat module\n");

        if (sysptr == NULL)
                sysptr = xconcat;

        return 0;
}

static void  __exit exit_sys_xconcat(void)
{
        if (sysptr != NULL)
                sysptr = NULL;

        printk(KERN_DEBUG"\nremoved sys_xconcat module\n");
}
module_init(init_sys_xconcat);
module_exit(exit_sys_xconcat);
MODULE_LICENSE("GPL");

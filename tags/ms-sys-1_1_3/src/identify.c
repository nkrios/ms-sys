#include <linux/hdreg.h>
#include <linux/fd.h>
#include <linux/fs.h>
#include <sys/ioctl.h>

#include "br.h"
#include "fat12.h"
#include "fat16.h"
#include "fat32.h"
#include "fat32nt.h"
#include "nls.h"
#include "identify.h"

/* This one should be in stdio.h, however it seems to be missing. Declared here
   to avoid warnings... */
int fileno( FILE *stream);


/* Returns TRUE if file is a device, otherwise FALSE */
static int is_disk_device(FILE *fp);

/* Returns TRUE if file is a floppy device, otherwise FALSE */
static int is_floppy(FILE *fp);

/* Returns TRUE if file is a partition device, otherwise FALSE */
static int is_partition(FILE *fp);

static int is_disk_device(FILE *fp)
{
   int iRes1;
   int iRes2;
   long lSectors;
   struct hd_geometry sGeometry;
   int iFd = fileno(fp);
   
   iRes1 = ioctl(iFd, BLKGETSIZE, &lSectors);
#ifdef HDIO_REQ
   iRes2 = ioctl(iFd, HDIO_REQ, &sGeometry);
#else
   iRes2 = ioctl(iFd, HDIO_GETGEO, &sGeometry);
#endif

   return ! (iRes1 && iRes2);
} /* is_device */

static int is_floppy(FILE *fp)
{
   struct floppy_struct sFloppy;
   
   return ! ioctl(fileno(fp) ,FDGETPRM, &sFloppy);
} /* is_floppy */

static int is_partition(FILE *fp)
{
   int iRes1;
   int iRes2;
   long lSectors;
   struct hd_geometry sGeometry;
   int iFd = fileno(fp);
   
   iRes1 = ioctl(iFd, BLKGETSIZE, &lSectors);
#ifdef HDIO_REQ
   iRes2 = ioctl(iFd, HDIO_REQ, &sGeometry);
#else
   iRes2 = ioctl(iFd, HDIO_GETGEO, &sGeometry);
#endif

   return (! (iRes1 && iRes2)) && (sGeometry.start);
} /* is_partition */

unsigned long partition_start_sector(FILE *fp)
{
   int iRes1;
   int iRes2;
   long lSectors;
   struct hd_geometry sGeometry;
   int iFd = fileno(fp);
   
   iRes1 = ioctl(iFd, BLKGETSIZE, &lSectors);
#ifdef HDIO_REQ
   iRes2 = ioctl(iFd, HDIO_REQ, &sGeometry);
#else
   iRes2 = ioctl(iFd, HDIO_GETGEO, &sGeometry);
#endif
   if(! (iRes1 && iRes2) )
      return sGeometry.start;
   else
      return 0L;
} /* partition_start_sector */

int sanity_check(FILE *fp, const char *szPath, int iBr, int bPrintMessages)
{
   int bIsDiskDevice = is_disk_device(fp);
   int bIsFloppy = is_floppy(fp);
   int bIsPartition = is_partition(fp);
   switch(iBr)
   {
      case MBR_2000:
      case MBR_95B:
      case MBR_DOS:
      case MBR_SYSLINUX:
      case MBR_ZERO:
      {
	 if( ! bIsDiskDevice )
	 {
	    if(bPrintMessages)
	    {
	       printf(_("%s does not seem to be a disk device,\n"), szPath);
	       printf(
		  _("use the switch -f to force writing of a master boot record\n"));
	    }
	    return 0;
	 }
	 if( bIsFloppy )
	 {
	    if(bPrintMessages)
	    {
	       printf(_("%s seems to be a floppy disk device,\n"), szPath);
	       printf(
		  _("use the switch -f to force writing of a master boot record\n"));
	    }
	    return 0;
	 }
	 if( bIsPartition )
	 {
	    if(bPrintMessages)
	    {
	       printf(_("%s seems to be a disk partition device,\n"), szPath);
	       printf(
		  _("use the switch -f to force writing of a master boot record\n"));
	    }
	    return 0;
	 }
      }
      break;
      case FAT12_BR:
      {
	 if( ! bIsFloppy )
	 {
	    if(bPrintMessages)
	    {
	       printf(_("%s does not seem to be a floppy disk device,\n"),
		      szPath);
	       printf(
		  _("use the switch -f to force writing of a FAT12 boot record\n"));
	    }
	    return 0;
	 }
	 if( ! is_fat_12_fs(fp))
	 {
	    if(bPrintMessages)
	    {
	       printf(_("%s does not seem to have a FAT12 file system,\n"),
		      szPath);
	       printf(
		  _("use the switch -f to force writing of a FAT12 boot record\n"));
	    }
	    return 0;
	 }
      }
      break;
      case FAT16_BR:
      {
	 if( ! bIsPartition )
	 {
	    if(bPrintMessages)
	    {
	       printf(_("%s does not seem to be a disk partition device,\n"),
		      szPath);
	       printf(
		  _("use the switch -f to force writing of a FAT16 boot record\n"));
	    }
	    return 0;
	 }
	 if( ! is_fat_16_fs(fp))
	 {
	    if(bPrintMessages)
	    {
	       printf(_("%s does not seem to have a FAT16 file system,\n"),
		      szPath);
	       printf(
		  _("use the switch -f to force writing of a FAT16 boot record\n"));
	    }
	    return 0;
	 }
      }
      break;
      case FAT32_BR:
      {
	 if( ! bIsPartition )
	 {
	    if(bPrintMessages)
	    {
	       printf(_("%s does not seem to be a disk partition device,\n"),
		      szPath);
	       printf(
		  _("use the switch -f to force writing of a FAT32 DOS boot record\n"));
	    }
	    return 0;
	 }
	 if( ! is_fat_32_fs(fp))
	 {
	    if(bPrintMessages)
	    {
	       printf(_("%s does not seem to have a FAT32 file system,\n"),
		      szPath);
	       printf(
		  _("use the switch -f to force writing of a FAT32 DOS boot record\n"));
	    }
	    return 0;
	 }
      }
      break;
      case FAT32NT_BR:
      {
	 if( ! bIsPartition )
	 {
	    if(bPrintMessages)
	    {
	       printf(_("%s does not seem to be a disk partition device,\n"),
		      szPath);
	       printf(
		  _("use the switch -f to force writing of a FAT32 NT boot record\n"));
	    }
	    return 0;
	 }
	 if( ! is_fat_32_fs(fp))
	 {
	    if(bPrintMessages)
	    {
	       printf(_("%s does not seem to have a FAT32 file system,\n"),
		      szPath);
	       printf(
		  _("use the switch -f to force writing of a FAT32 NT boot record\n"));
	    }
	    return 0;
	 }
      }
      break;
      default:
      {
	 if(bPrintMessages)
	 {
	    printf(_("Whoops, internal error, unknown boot record\n"));
	 }
	 return 0;
      }
      break;
   }
   return 1;
} /* sanity_check */

void diagnose(FILE *fp, const char *szPath)
{
   if(is_fat_12_fs(fp))
      printf(_("%s has a FAT12 file system.\n"), szPath);
   if(is_fat_16_fs(fp))
      printf(_("%s has a FAT16 file system.\n"), szPath);
   if(is_fat_32_fs(fp))
      printf(_("%s has a FAT32 file system.\n"), szPath);
   if(is_br(fp))
      printf(_("%s has an x86 boot sector,\n"), szPath);
   else
   {
      printf(_("%s has no x86 boot sector\n"), szPath);
      return;
   }
   if(entire_fat_12_br_matches(fp))
   {
      printf(
	 _("it is exactly the kind of FAT12 boot record this program\n"));
      printf(
	 _("would create with the switch -1 on a floppy.\n"));
   }
   else if(is_fat_16_br(fp) || is_fat_32_br(fp))
   {
      if(entire_fat_16_br_matches(fp))
      {
	 printf(
	    _("it is exactly the kind of FAT16 boot record this program\n"));
	 printf(
	    _("would create with the switch -6 on a FAT16 partition.\n"));
      }
      else if(entire_fat_32_br_matches(fp))
      {
	 printf(
	  _("it is exactly the kind of FAT32 DOS boot record this program\n"));
	 printf(
	    _("would create with the switch -3 on a FAT32 partition.\n"));
      }
      else if(entire_fat_32_nt_br_matches(fp))
      {
	 printf(
	   _("it is exactly the kind of FAT32 NT boot record this program\n"));
	 printf(
	    _("would create with the switch -2 on a FAT32 partition.\n"));
      }
      else
      {
	 printf(
	    _("it seems to be a FAT16 or FAT32 boot record, but it\n"));
	 printf(
	    _("differs from what this program would create with the\n"));
	 printf(_("switch -6, -2 or -3 on a FAT16 or FAT32 partition.\n"));
      }
   } 
   else if(is_lilo_br(fp))
   {
      printf(_("it seems to be a LILO boot record, please use lilo to\n"));
      printf(_("create such boot records.\n"));
   }
   else if(is_dos_mbr(fp))
   {
	 printf(
	    _("it is a Microsoft DOS/NT/95A master boot record, like the one this\n"));
	 printf(
	    _("program creates with the switch -d on a hard disk device.\n"));
   }
   else if(is_95b_mbr(fp))
   {
	 printf(
	    _("it is a Microsoft 95B/98/98SE/ME master boot record, like the one this\n"));
	 printf(
	    _("program creates with the switch -9 on a hard disk device.\n"));
   }
   else if(is_2000_mbr(fp))
   {
	 printf(
	    _("it is a Microsoft 2000/XP/2003 master boot record, like the one this\n"));
	 printf(
	    _("program creates with the switch -m on a hard disk device.\n"));
   }
   else if(is_syslinux_mbr(fp))
   {
	 printf(
	    _("it is a public domain syslinux master boot record, like the one this\n"));
	 printf(
	    _("program creates with the switch -s on a hard disk device.\n"));
   }
   else if(is_zero_mbr(fp))
   {
	 printf(
	    _("it is a zeroed non-bootable master boot record, like the one this\n"));
	 printf(
	    _("program creates with the switch -z on a hard disk device.\n"));
   }
   else
      printf(_("it is an unknown boot record\n"));
} /* diagnose */

int smart_select(FILE *fp)
{
   int i;

   for(i=AUTO_BR+1; i<NUMBER_OF_RECORD_TYPES; i++)
      if(sanity_check(fp, "", i, 0))
	 return i;
   return NO_WRITING;
} /* smart_select */

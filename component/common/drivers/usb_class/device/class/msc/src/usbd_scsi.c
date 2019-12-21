#include "msc/inc/usbd_scsi.h"

uint8_t Page00_Inquiry_Data[] =
  {
    0x00, /* PERIPHERAL QUALIFIER & PERIPHERAL DEVICE TYPE*/
    0x00,
    0x00,
    0x00,
    0x00 /* Supported Pages 00*/
  };

uint8_t Standard_Inquiry_Data[] =
  {
    0x00,          /* Direct Access Device */
    0x80,          /* RMB = 1: Removable Medium */
    0x02,          /* Version: No conformance claim to standard */
    0x02,

    36 - 4,          /* Additional Length */
    0x00,          /* SCCS = 1: Storage Controller Component */
    0x00,
    0x00,
    /* Vendor Identification */
    'R', 'e', 'a', 'l', 'T', 'e', 'k', ' ',
    /* Product Identification */
    'S', 'D', ' ', 'F', 'l', 'a', 's', 'h', ' ',
    'D', 'i', 's', 'k', ' ', ' ', ' ',
    /* Product Revision Level */
    '1', '.', '0', ' '
  };

uint8_t Mode_Sense6_data[] =
  {
    0x03,
    0x00,
    0x00,
    0x00,
  };

uint8_t Mode_Sense10_data[] =
  {
    0x00,
    0x06,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00
  };

static int usbd_msc_scsi_send_status(struct msc_common *common)
{
	struct msc_lun	*curlun = common->curlun;
	struct msc_bufhd* bufhd = NULL;
	struct usb_request *reqin = NULL;
	struct bulk_cs_wrap	*csw;
	u8			status = US_BULK_STAT_OK;
	u32			sd; // sense data

	bufhd = common->csw_bh;
	reqin = bufhd->reqin;
	
	if (curlun){
		sd = curlun->sense_data;
	} else if (common->bad_lun_okay)
		sd = SS_NO_SENSE;
	else
		sd = SS_LOGICAL_UNIT_NOT_SUPPORTED;

	if (common->phase_error) {
		USBD_ERROR("sending phase-error status\n");
		status = US_BULK_STAT_PHASE;
		sd = SS_INVALID_COMMAND;
	} else if (sd != SS_NO_SENSE) {
		USBD_WARN("sending command-failure (0x%2X) status\n", common->scsi_cmnd[0]);
		status = US_BULK_STAT_FAIL;
	}

	/* Store and send the Bulk-only CSW */
	csw = (void *)reqin->buf;

	csw->Signature = rtk_cpu_to_le32(US_BULK_CS_SIGN);
	csw->Tag = common->tag;
	csw->Residue = rtk_cpu_to_le32(common->residue);
	csw->Status = status;

	reqin->length = US_BULK_CS_WRAP_LEN;
	reqin->zero = 0;
//	reqin->no_interrupt = 1;

	if (usbd_msc_bulk_in_transfer(common->mscdev, reqin))
		/* Don't know what to do if common->fsg is NULL */
		return -EIO;

	return 0;
}


static int usbd_msc_verify(struct msc_common *common)
{
#if 0
	struct msc_lun		*curlun = common->curlun;
	u32			lba;
	u32			verification_length;
	u32			file_offset, file_offset_tmp;
	u32			amount_left;
	unsigned int		amount;
	u32			nread;

	lba = common->scsi_cmnd[2]<<24 | common->scsi_cmnd[3] << 16 |
			common->scsi_cmnd[4]<<8|common->scsi_cmnd[5];
	if (lba >= curlun->num_sectors) {
		curlun->sense_data = SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;
		return -EINVAL;
	}

	/*
	 * We allow DPO (Disable Page Out = don't save data in the
	 * cache) but we don't implement it.
	 */
	if (common->scsi_cmnd[1] & ~0x10) {
		curlun->sense_data = SS_INVALID_FIELD_IN_CDB;
		return -EINVAL;
	}

	verification_length = common->scsi_cmnd[7]<<8|common->scsi_cmnd[8];
	if (unlikely(verification_length == 0))
		return -EIO;		/* No default reply */

	/* Prepare to carry out the file verify */
	amount_left = verification_length << curlun->blkbits;
	file_offset = ((u32) lba) << curlun->blkbits;

	/* Just try to read the requested blocks */
	// TODO:
#endif
	return 0;
}

static int usbd_msc_scsi_inquiry(struct msc_common *common)
{
	struct msc_lun *curlun = common->curlun;
	struct usb_request *reqin;
	struct msc_bufhd* bufhd;
	u8 *ptr_str;
	u8 inquiry_len;

	bufhd = usbd_msc_get_bufhd(common);

	common->curbh = bufhd; // store the current buffer header for data stage

	reqin = bufhd->reqin;

	if (!curlun) {		/* Unsupported LUNs are okay */
		common->bad_lun_okay = 1;
		rtw_memset(reqin->buf, 0, 36);
		((u8*)reqin->buf)[0] = 0x7f;		/* Unsupported, no device-type */
		((u8*)reqin->buf)[4] = 31;		/* Additional length */
		return STANDARD_INQUIRY_DATA_LEN;
	}

#if 0
	if (common->scsi_cmnd[1] & 0x01)/*Evpd is set*/
	{
		ptr_str = (u8 *)Page00_Inquiry_Data;
		inquiry_len = 5;
	}
	else{
		ptr_str = (u8 *)Standard_Inquiry_Data;
		if (common->scsi_cmnd[4] <= STANDARD_INQUIRY_DATA_LEN)
      		inquiry_len = common->scsi_cmnd[4];
		else
			inquiry_len = STANDARD_INQUIRY_DATA_LEN;
	}
#else
	ptr_str = (u8 *)Standard_Inquiry_Data;
	if (common->scsi_cmnd[4] <= STANDARD_INQUIRY_DATA_LEN)
  		inquiry_len = common->scsi_cmnd[4];
	else
		inquiry_len = STANDARD_INQUIRY_DATA_LEN;
#endif
	rtw_memcpy(reqin->buf, ptr_str, inquiry_len);

	return inquiry_len;
}

static int usbd_msc_read_format_capacities(struct msc_common *common){
	struct msc_lun	*curlun = common->curlun;
	struct usb_request *reqin;
	u8* buf = NULL;
	struct msc_bufhd* bufhd;

	bufhd = usbd_msc_get_bufhd(common);

	common->curbh = bufhd; // store the current buffer header for data stage
	reqin = bufhd->reqin;
	buf = (u8*)reqin->buf;

	buf[0] = buf[1] = buf[2] = 0;
	buf[3] = 8; /* Only the Current/Maximum Capacity Descriptor */

	/* Block Count */
	buf[4] = (curlun->num_sectors >> 24) & 0xFF;
	buf[5] = (curlun->num_sectors >> 16) & 0xFF;
	buf[6] = (curlun->num_sectors >>  8) & 0xFF;
	buf[7] = (curlun->num_sectors >>  0) & 0xFF;

	/* Block Length */
	buf[8] = 0x02;						/* Descriptor Code: Formatted Media */
	buf[9] = (curlun->blksize  >> 16) & 0xFF;
	buf[10] = (curlun->blksize  >>  8) & 0xFF;
	buf[11] = (curlun->blksize  >>  0) & 0xFF;

	return 12;
}


static int usbd_msc_request_sense(struct msc_common *common)
{
	struct msc_lun	*curlun = common->curlun;
	struct usb_request *reqin;
	struct msc_bufhd* bufhd;
	u8* buf	= NULL;
	u32		sd, sdinfo;
	int		valid;

	bufhd = usbd_msc_get_bufhd(common);
	common->curbh = bufhd; // store the current buffer header for data stage

	reqin = bufhd->reqin;
	buf = (u8*)reqin->buf;

	/*
	 * From the SCSI-2 spec., section 7.9 (Unit attention condition):
	 *
	 * If a REQUEST SENSE command is received from an initiator
	 * with a pending unit attention condition (before the target
	 * generates the contingent allegiance condition), then the
	 * target shall either:
	 *   a) report any pending sense data and preserve the unit
	 *	attention condition on the logical unit, or,
	 *   b) report the unit attention condition, may discard any
	 *	pending sense data, and clear the unit attention
	 *	condition on the logical unit for that initiator.
	 *
	 * FSG normally uses option a); enable this code to use option b).
	 */

	if (curlun && curlun->unit_attention_data != SS_NO_SENSE) {
		curlun->sense_data = curlun->unit_attention_data;
		curlun->unit_attention_data = SS_NO_SENSE;
	}

	if (!curlun) {		/* Unsupported LUNs are okay */
		common->bad_lun_okay = 1;
		sd = SS_LOGICAL_UNIT_NOT_SUPPORTED;
		sdinfo = 0;
		valid = 0;
	} else {
		sd = curlun->sense_data;
		sdinfo = curlun->sense_data_info;
		valid = curlun->info_valid << 7;
		curlun->sense_data = SS_NO_SENSE;
		curlun->sense_data_info = 0;
		curlun->info_valid = 0;
	}

	rtw_memset(buf, 0x00, 18);
	buf[0] = valid | 0x70;			/* Valid, current error */
	buf[2] = SK(sd);

	buf[3] = (sdinfo >> 24) & 0xFF;
	buf[4] = (sdinfo >> 16) & 0xFF;
	buf[5] = (sdinfo >>  8) & 0xFF;
	buf[6] = (sdinfo >>  0) & 0xFF;

	buf[7] = 18 - 8;			/* Additional sense length */
	buf[12] = ASC(sd);
	buf[13] = ASCQ(sd);
	return 18;
}

static int usbd_msc_mode_sense(struct msc_common *common)
{
	struct msc_lun	*curlun = common->curlun;
	struct usb_request *reqin;
	struct msc_bufhd* bufhd;
	u8* buf	= NULL;
	int		mscmnd = common->scsi_cmnd[0];
	int		len;

	bufhd = usbd_msc_get_bufhd(common);
	common->curbh = bufhd; // store the current buffer header for data stage

	reqin = bufhd->reqin;
	buf = (u8*)reqin->buf;
	
	if ((common->scsi_cmnd[1] & ~0x08) != 0) {	/* Mask away DBD */
		curlun->sense_data = SS_INVALID_FIELD_IN_CDB;
		return -EINVAL;
	}
#if 0
	pc = common->scsi_cmnd[2] >> 6;
	page_code = common->scsi_cmnd[2] & 0x3f;
	if (pc == 3) {
		curlun->sense_data = SS_SAVING_PARAMETERS_NOT_SUPPORTED;
		return -EINVAL;
	}
	changeable_values = (pc == 1);
	all_pages = (page_code == 0x3f);

	/*
	 * Write the mode parameter header.  Fixed values are: default
	 * medium type, no cache control (DPOFUA), and no block descriptors.
	 * The only variable value is the WriteProtect bit.  We will fill in
	 * the mode data length later.
	 */
	rtw_memset(buf, 0, 8);
	if (mscmnd == SCSI_MODE_SENSE6) {
		buf[2] = (curlun->ro ? 0x80 : 0x00);		/* WP, DPOFUA */
		buf += 4;
		limit = 255;
	} else {			/* MODE_SENSE_10 */
		buf[3] = (curlun->ro ? 0x80 : 0x00);		/* WP, DPOFUA */
		buf += 8;
		limit = 65535;		/* Should really be FSG_BUFLEN */
	}

	/* No block descriptors */

	/*
	 * The mode pages, in numerical order.  The only page we support
	 * is the Caching page.
	 */
	if (page_code == 0x08 || all_pages) {
		valid_page = 1;
		buf[0] = 0x08;		/* Page code */
		buf[1] = 10;		/* Page length */
		rtw_memset(buf+2, 0x00, 10);	/* None of the fields are changeable */

		if (!changeable_values) {
			buf[2] = 0x04;	/* Write cache enable, */
					/* Read cache not disabled */
					/* No cache retention priorities */
			buf[4] = 0xFF;
			buf[5] = 0xFF;
					/* Don't disable prefetch */
					/* Minimum prefetch = 0 */
			buf[8] = 0xFF;
			buf[9] = 0xFF;
					/* Maximum prefetch */
			buf[10] = 0xFF;
			buf[11] = 0xFF;
					/* Maximum prefetch ceiling */
		}
		buf += 12;
	}

	/*
	 * Check that a valid page was requested and the mode data length
	 * isn't too long.
	 */
	len = buf - buf0;
	if (!valid_page || len > limit) {
		curlun->sense_data = SS_INVALID_FIELD_IN_CDB;
		return -EINVAL;
	}

	/*  Store the mode data length */
	if (mscmnd == SCSI_MODE_SENSE6)
		buf0[0] = len - 1;
	else{
		buf0[0] = ((len - 2) >>  8) & 0xFF;
		buf0[1] = ((len - 2) >>  0) & 0xFF;
	}
#else
	if (mscmnd == SCSI_MODE_SENSE6){
		rtw_memcpy(buf, Mode_Sense6_data, sizeof(Mode_Sense6_data));
		len = sizeof(Mode_Sense6_data);
	}else{
		rtw_memcpy(buf, Mode_Sense10_data, sizeof(Mode_Sense10_data));
		len = sizeof(Mode_Sense10_data);
	}
#endif
	return len;
}

static int usbd_msc_mode_select(struct msc_common *common)
{
	struct msc_lun	*curlun = common->curlun;

	/* We don't support MODE SELECT */
	if (curlun)
		curlun->sense_data = SS_INVALID_COMMAND;
	return -EINVAL;
}

static int usbd_msc_read_capacity(struct msc_common *common)
{
	struct msc_lun	*curlun = common->curlun;
	struct usb_request *reqin;
	struct msc_bufhd* bufhd;
	u8* buf	= NULL;
	u32		lba;// = get_unaligned_be32(&common->cmnd[2]);
	int		pmi = common->scsi_cmnd[8];
	u32	sectors = curlun->num_sectors;
	u32 blksize = curlun->blksize;

	bufhd = usbd_msc_get_bufhd(common);
	common->curbh = bufhd; // store the current buffer header for data stage

	reqin = bufhd->reqin;
	buf = (u8*)reqin->buf;

	lba = common->scsi_cmnd[2]<<24 | common->scsi_cmnd[3]<<16
		| common->scsi_cmnd[4]<<8 | common->scsi_cmnd[5];

	/* Check the PMI and LBA fields */
	if (pmi > 1 || (pmi == 0 && lba != 0)) {
		curlun->sense_data = SS_INVALID_FIELD_IN_CDB;
		return -EINVAL;
	}

	/* Block Count */
	buf[0] = ((sectors-1) >> 24) & 0xFF;
	buf[1] = ((sectors-1) >> 16) & 0xFF;
	buf[2] = ((sectors-1) >>  8) & 0xFF;
	buf[3] = ((sectors-1) >>  0) & 0xFF;

	/* Block Length */
	buf[4] = (blksize >> 24) & 0xFF;
	buf[5] = (blksize >> 16) & 0xFF;
	buf[6] = (blksize >> 8) & 0xFF;
	buf[7] = (blksize >> 0) & 0xFF;
	
	return 8;
}
extern int usbd_msc_lun_read(struct msc_lun *curlun, u32 sector,u8 *buffer,u32 count);
extern int usbd_msc_lun_write(struct msc_lun *curlun, u32 sector,const u8 *buffer,u32 count);

static int usbd_msc_read(struct msc_common *common)
{
	struct msc_lun	*curlun = common->curlun;
	struct msc_bufhd *bufhd;
	u32	lba;	// logical block address
	u32 blkNbr; // block number
	int	rc;
	u32	blks_left_to_read; /*amount in block number to be read */
	u32	nread;

	/*
	 * Get the starting Logical Block Address and check that it's
	 * not too big.
	 */
	if (common->scsi_cmnd[0] == SCSI_READ6)
		lba = common->scsi_cmnd[1]<<16 | common->scsi_cmnd[2]<<8 | common->scsi_cmnd[3];
	else {
		lba = common->scsi_cmnd[2]<<24 | common->scsi_cmnd[3]<<16
				| common->scsi_cmnd[4]<<8 | common->scsi_cmnd[5];

		/*
		 * We allow DPO (Disable Page Out = don't save data in the
		 * cache) and FUA (Force Unit Access = don't read from the
		 * cache), but we don't implement them.
		 */
		if ((common->scsi_cmnd[1] & ~0x18) != 0) {
			curlun->sense_data = SS_INVALID_FIELD_IN_CDB;
			return -EINVAL;
		}
	}
	if (lba >= curlun->num_sectors) {
		curlun->sense_data = SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;
		return -EINVAL;
	}
	
	blks_left_to_read = common->data_size_from_cmnd>>curlun->blkbits;
	if (unlikely(blks_left_to_read == 0))
		return -EIO;		/* No default reply */

	for (;;) {
		/*
		 * If we were asked to read over addressed,
		 * end with an empty buffer.
		 */
		if (lba > curlun->num_sectors) {
			curlun->sense_data =
					SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;
			curlun->sense_data_info = lba;
			curlun->info_valid = 1;
			bufhd->reqin->length = 0;
			break;
		}

		bufhd = usbd_msc_get_bufhd(common);
		if(bufhd == NULL){
			rtw_mdelay_os(1);
			continue;
		}
		
		/* read only one block*/
		blkNbr = min(blks_left_to_read, bufhd->buf_size>>curlun->blkbits);

retry:
		// read operation
		if(blkNbr >= 1){
			rc = usbd_msc_lun_read(curlun, lba, bufhd->buf, blkNbr);
			//rc = curlun->lun_opts->disk_read(lba, bufhd->buf, blkNbr);
		if(rc){
			USBD_ERROR("error in file read:sector(%d),counts(%d)\n", lba, blkNbr);
			goto retry;
		}
		}else{
			USBD_ERROR("invalid parameter:sector(%d),counts(%d)\n", lba, blkNbr);
		}

		/*update logical block address*/
		lba += blkNbr;
		blks_left_to_read -= blkNbr;
		
		nread = blkNbr << curlun->blkbits;

		common->residue -= nread;
		
		bufhd->reqin->length = nread;
		
		//USBD_PRINTF("bufhd(No):%d, common->residue %d\n",bufhd->reqin->length,common->residue);

		/* Send this buffer and go read some more */
		bufhd->reqin->zero = 0;
		if (usbd_msc_bulk_in_transfer(common->mscdev, bufhd->reqin))
			/* Don't know what to do if common->fsg is NULL */
			return -EIO;
		
		if (blks_left_to_read == 0){
			break;		/* No more left to read */
		}
		
	}

	return -EIO;		/* No default reply */
}



/*-------------------------------------------------------------------------*/
// Config use thread to write data
#define CONGIF_USE_DATA_THREAD	0

static int usbd_msc_write(struct msc_common *common)
{
	struct msc_lun		*curlun = common->curlun;
	u32	lba;
	int	get_some_more;
	u32	amount_left_to_req, amount_left_to_write;
	u32	amount;
#if (CONGIF_USE_DATA_THREAD==0)
	int	rc;
	u32 blkNbr; // block number
	u32	nwritten;
#endif
	struct msc_bufhd	*bufhd;

	if (curlun->ro) {
		curlun->sense_data = SS_WRITE_PROTECTED;
		return -EINVAL;
	}

	/*
	 * Get the starting Logical Block Address and check that it's
	 * not too big
	 */
	if (common->scsi_cmnd[0] == SCSI_WRITE6)
			lba = common->scsi_cmnd[1]<<16 | common->scsi_cmnd[2]<<8 | common->scsi_cmnd[3];
		else {
			lba = common->scsi_cmnd[2]<<24 | common->scsi_cmnd[3]<<16
					| common->scsi_cmnd[4]<<8 | common->scsi_cmnd[5];

		/*
		 * We allow DPO (Disable Page Out = don't save data in the
		 * cache) and FUA (Force Unit Access = write directly to the
		 * medium).  We don't implement DPO; we implement FUA by
		 * performing synchronous output.
		 */
		if (common->scsi_cmnd[1] & ~0x18) {
			curlun->sense_data = SS_INVALID_FIELD_IN_CDB;
			return -EINVAL;
		}
		if (!curlun->nofua && (common->scsi_cmnd[1] & 0x08)) { /* FUA */
			//spin_lock(&curlun->filp->f_lock);
			//curlun->filp->f_flags |= O_SYNC;
			//spin_unlock(&curlun->filp->f_lock);
		}
	}
	if (lba >= curlun->num_sectors) {
		curlun->sense_data = SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;
		return -EINVAL;
	}

	/* Carry out the file writes */
	get_some_more = 1;
	amount_left_to_req = common->data_size_from_cmnd;
	if (unlikely(amount_left_to_req == 0))
		return -EIO;		/* No default reply */
	amount_left_to_write = amount_left_to_req;
	curlun->lba = lba;
//	USBD_PRINTF("lba (%d) amount_left_to_req (%d)\n",lba, amount_left_to_req);

	while (1) {
		/* submit request */
		if(get_some_more){
			/* get a buffer head to get data from host */
			bufhd = usbd_msc_get_bufhd(common);
			if(bufhd == NULL){
				rtw_yield_os();
				rtw_mdelay_os(1);
				goto check_data;
			}
			amount = min(amount_left_to_req, bufhd->buf_size);

			if (lba > curlun->num_sectors) {
				curlun->sense_data =
					SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;
				curlun->sense_data_info = lba;
				curlun->info_valid = 1;
				bufhd->reqin->length = 0;
				break;
			}

			bufhd->reqout->length = amount;
			if(usbd_msc_bulk_out_transfer(common->mscdev, bufhd->reqout))
				return -EIO;

			common->usb_amount_left -= amount;
			amount_left_to_req -= amount;
			if (amount_left_to_req == 0){
				get_some_more = 0;
#if (CONGIF_USE_DATA_THREAD==1)
				break; // break out when all request submited done
#endif
			}
			/* continue submit bulk out request to receive data from host */
			continue;
		}

check_data:
#if (CONGIF_USE_DATA_THREAD==0)
		/* wait data to come */
		rtw_mutex_get(&common->bod_mutex);
		rc = rtw_is_list_empty(&common->bod_list);
		rtw_mutex_put(&common->bod_mutex);
		if(rc == _TRUE){
			rtw_mdelay_os(1);
			goto check_data;
		}
		/* write data to disk */
		rtw_mutex_get(&common->bod_mutex);
		bufhd = list_first_entry(&common->bod_list, struct msc_bufhd, list);
		rtw_mutex_put(&common->bod_mutex);

		if(bufhd->type!=BUFHD_DATA){
			rtw_mutex_get(&common->bod_mutex);
			rtw_list_delete(&bufhd->list);
			rtw_mutex_put(&common->bod_mutex);
			usbd_msc_put_bufhd(common, bufhd);
			continue; 
		}

		amount = bufhd->reqout->actual;
		blkNbr = amount >> curlun->blkbits;
		if(blkNbr >= 1){
			rc = usbd_msc_lun_write(common->curlun, lba, bufhd->buf, blkNbr);
			//rc = curlun->lun_opts->disk_write(lba, bufhd->buf, blkNbr);
		if(rc){
			USBD_ERROR("error in file write:sector(%d),counts(%d)\n", lba, blkNbr);
			continue;
		}
			//USBD_ERROR("write: sector %d(%d)", lba, blkNbr);
		}else{
			USBD_ERROR("invalid parameter:sector(%d),counts(%d)\n", lba, blkNbr);
		}
		
		rtw_mutex_get(&common->bod_mutex);
		rtw_list_delete(&bufhd->list);
		rtw_mutex_put(&common->bod_mutex);
		
		usbd_msc_put_bufhd(common, bufhd);

		lba += blkNbr;
		nwritten = amount;
		amount_left_to_write -= nwritten;
		common->residue -= nwritten;

		if(amount_left_to_write == 0)
			break;
#endif
		/* check get more data again*/
		continue;
	}
	return -EIO;		/* No default reply */
}


static int usbd_msc_prevent_allow(struct msc_common *common)
{
	struct msc_lun	*curlun = common->curlun;
	int		prevent;

	if (!common->curlun) {
		return -EINVAL;
	} else if (!common->curlun->removable) {
		common->curlun->sense_data = SS_INVALID_COMMAND;
		return -EINVAL;
	}


	prevent = common->scsi_cmnd[4] & 0x01;
	if ((common->scsi_cmnd[4] & ~0x01) != 0) {	/* Mask away Prevent */
		curlun->sense_data = SS_INVALID_FIELD_IN_CDB;
		return -EINVAL;
	}

//	if (curlun->prevent_medium_removal && !prevent)
//		fsg_lun_fsync_sub(curlun);

	curlun->prevent_medium_removal = prevent;

	return 0;
}

static int usbd_msc_start_stop(struct msc_common *common)
{
	struct msc_lun	*curlun = common->curlun;
	int		loej, start;

	if (!curlun) {
		return -EINVAL;
	} else if (!curlun->removable) {
		curlun->sense_data = SS_INVALID_COMMAND;
		return -EINVAL;
	} else if ((common->scsi_cmnd[1] & ~0x01) != 0 || /* Mask away Immed */
		   (common->scsi_cmnd[4] & ~0x03) != 0) { /* Mask LoEj, Start */
		curlun->sense_data = SS_INVALID_FIELD_IN_CDB;
		return -EINVAL;
	}

	loej  = common->scsi_cmnd[4] & 0x02;
	start = common->scsi_cmnd[4] & 0x01;

	/*
	 * Our emulation doesn't support mounting; the medium is
	 * available for use as soon as it is loaded.
	 */
	if (start) {
		if (!(curlun->is_open)) {
			curlun->sense_data = SS_MEDIUM_NOT_PRESENT;
			return -EINVAL;
		}
		return 0;
	}

	if (!loej)
		return 0;

	// mark the lun is closed
	curlun->is_open = 0;
	return 0;
}

/*
 * Check whether the command is properly formed and whether its data size
 * and direction agree with the values we already have.
 */
static int usbd_msc_check_command(struct msc_common *common, int cmnd_size,
			 enum data_direction data_dir, unsigned int mask,
			 int needs_medium, const char *name)
{
	int			i;
	unsigned int		lun = common->scsi_cmnd[1] >> 5;
	struct msc_lun		*curlun;

	/*
	 * We can't reply at all until we know the correct data direction
	 * and size.
	 */
	if (common->data_size_from_cmnd == 0)
		data_dir = DATA_DIR_NONE;
	if (common->data_size < common->data_size_from_cmnd) {
		/*
		 * Host data size < Device data size is a phase error.
		 * Carry out the command, but only transfer as much as
		 * we are allowed.
		 */
		common->data_size_from_cmnd = common->data_size;
		common->phase_error = 1;
	}
	common->residue = common->data_size;
	common->usb_amount_left = common->data_size;

	/* Conflicting data directions is a phase error */
	if (common->data_dir != data_dir && common->data_size_from_cmnd > 0) {
		common->phase_error = 1;
		return -EINVAL;
	}

	/* Verify the length of the command itself */
	if (cmnd_size != common->cmnd_size) {

		/*
		 * Special case workaround: There are plenty of buggy SCSI
		 * implementations. Many have issues with cbw->Length
		 * field passing a wrong command size. For those cases we
		 * always try to work around the problem by using the length
		 * sent by the host side provided it is at least as large
		 * as the correct command length.
		 * Examples of such cases would be MS-Windows, which issues
		 * REQUEST SENSE with cbw->Length == 12 where it should
		 * be 6, and xbox360 issuing INQUIRY, TEST UNIT READY and
		 * REQUEST SENSE with cbw->Length == 10 where it should
		 * be 6 as well.
		 */
		if (cmnd_size <= common->cmnd_size) {
			USBD_WARN("%s is buggy! Expected length %d "
			    "but we got %d\n", name,
			    cmnd_size, common->cmnd_size);
			cmnd_size = common->cmnd_size;
		} else {
			common->phase_error = 1;
			return -EINVAL;
		}
	}

	/* Check that the LUN values are consistent */
	if (common->lun != lun)
		USBD_ERROR("using LUN %u from CBW, not LUN %u from CDB\n",
		    common->lun, lun);

	/* Check the LUN */
	curlun = common->curlun;
	if (curlun) {
		if (common->scsi_cmnd[0] != SCSI_REQUEST_SENSE) {
			curlun->sense_data = SS_NO_SENSE;
			curlun->sense_data_info = 0;
			curlun->info_valid = 0;
		}
	} else {
		common->bad_lun_okay = 0;

		/*
		 * INQUIRY and REQUEST SENSE commands are explicitly allowed
		 * to use unsupported LUNs; all others may not.
		 */
		if (common->scsi_cmnd[0] != SCSI_INQUIRY &&
		    common->scsi_cmnd[0] != SCSI_REQUEST_SENSE) {
			USBD_ERROR("unsupported LUN %u\n", common->lun);
			return -EINVAL;
		}
	}

	/*
	 * If a unit attention condition exists, only INQUIRY and
	 * REQUEST SENSE commands are allowed; anything else must fail.
	 */
	if (curlun && curlun->unit_attention_data != SS_NO_SENSE &&
	    common->scsi_cmnd[0] != SCSI_INQUIRY &&
	    common->scsi_cmnd[0] != SCSI_REQUEST_SENSE) {
		curlun->sense_data = curlun->unit_attention_data;
		curlun->unit_attention_data = SS_NO_SENSE;
		return -EINVAL;
	}

	/* Check that only command bytes listed in the mask are non-zero */
	common->scsi_cmnd[1] &= 0x1f;			/* Mask away the LUN */
	for (i = 1; i < cmnd_size; ++i) {
		if (common->scsi_cmnd[i] && !(mask & (1 << i))) {
			if (curlun)
				curlun->sense_data = SS_INVALID_FIELD_IN_CDB;
			return -EINVAL;
		}
	}
	
	/*check LUN is open or not*/
	if (curlun && (!curlun->is_open) && needs_medium) {
		curlun->sense_data = SS_MEDIUM_NOT_PRESENT;
		return -EINVAL;
	}
	
	return 0;
}

static int check_command_size_in_blocks(struct msc_common *common,
		int cmnd_size, enum data_direction data_dir,
		unsigned int mask, int needs_medium, const char *name)
{
	if (common->curlun)
		common->data_size_from_cmnd <<= common->curlun->blkbits;
	return usbd_msc_check_command(common, cmnd_size, data_dir,
			mask, needs_medium, name);
}


int usbd_msc_scsi_cmd_handler(struct msc_common *common){
	int 		reply = -EINVAL;
	int 		i;
	static char 	unknown[16];

	common->phase_error = 0;
	common->short_packet_received = 0;

	switch (common->scsi_cmnd[0]) {

	case SCSI_INQUIRY:
		common->data_size_from_cmnd = common->scsi_cmnd[4];
		reply = usbd_msc_check_command(common, 6, DATA_DIR_TO_HOST,
					  (1<<4), 0,
					  "SCSI_INQUIRY");
		if (reply == 0)
			reply = usbd_msc_scsi_inquiry(common);
		break;
	case SCSI_MODE_SELECT6:
		common->data_size_from_cmnd = common->scsi_cmnd[4];
		reply = usbd_msc_check_command(common, 6, DATA_DIR_FROM_HOST,
					  (1<<1) | (1<<4), 0,
					  "SCSI_MODE_SELECT(6)");
		if (reply == 0)
			reply = usbd_msc_mode_select(common);
		break;

	case SCSI_MODE_SELECT10:
		common->data_size_from_cmnd = common->scsi_cmnd[7]<<8 | common->scsi_cmnd[8];
		reply = usbd_msc_check_command(common, 10, DATA_DIR_FROM_HOST,
					  (1<<1) | (3<<7), 0,
					  "SCSI_MODE_SELECT(10)");
		if (reply == 0)
			reply = usbd_msc_mode_select(common);
		break;
	case SCSI_MODE_SENSE6:
		common->data_size_from_cmnd = common->scsi_cmnd[4];
		reply = usbd_msc_check_command(common, 6, DATA_DIR_TO_HOST,
					  (1<<1) | (1<<2) | (1<<4), 0,
					  "SCSI_MODE_SENSE(6)");
		if (reply == 0)
			reply = usbd_msc_mode_sense(common);
		break;

	case SCSI_MODE_SENSE10:
		common->data_size_from_cmnd = common->scsi_cmnd[7]<<8 | common->scsi_cmnd[8];
		reply = usbd_msc_check_command(common, 10, DATA_DIR_TO_HOST,
					  (1<<1) | (1<<2) | (3<<7), 0,
					  "SCSI_MODE_SENSE(10)");
		if (reply == 0)
			reply = usbd_msc_mode_sense(common);
		break;

	case SCSI_ALLOW_MEDIUM_REMOVAL:
		common->data_size_from_cmnd = 0;
		reply = usbd_msc_check_command(common, 6, DATA_DIR_NONE,
					  (1<<4), 0,
					  "PREVENT-ALLOW MEDIUM REMOVAL");
		if (reply == 0)
			reply = usbd_msc_prevent_allow(common);
		break;
		
	case SCSI_READ6:
		i = common->scsi_cmnd[4];
		common->data_size_from_cmnd = (i == 0) ? 256 : i;
		reply = check_command_size_in_blocks(common, 6,
					  DATA_DIR_TO_HOST,
					  (7<<1) | (1<<4), 1,
					  "SCSI_READ(6)");
		if (reply == 0)
			reply = usbd_msc_read(common);
		break;

	case SCSI_READ10:
		common->data_size_from_cmnd = common->scsi_cmnd[7]<<8 | common->scsi_cmnd[8];
		reply = check_command_size_in_blocks(common, 10,
					  DATA_DIR_TO_HOST,
					  (1<<1) | (0xf<<2) | (3<<7), 1,
					  "SCSI_READ(10)");
		if (reply == 0)
			reply = usbd_msc_read(common);
		break;

	case SCSI_READ12:
		common->data_size_from_cmnd =
				common->scsi_cmnd[6]<<24 | common->scsi_cmnd[7] << 16 |common->scsi_cmnd[8]<<8|common->scsi_cmnd[9];
		reply = check_command_size_in_blocks(common, 12,
					  DATA_DIR_TO_HOST,
					  (1<<1) | (0xf<<2) | (0xf<<6), 1,
					  "SCSI_READ(12)");
		if (reply == 0)
			reply = usbd_msc_read(common);
		break;

	case SCSI_READ_CAPACITY10:
		common->data_size_from_cmnd = 8;
		reply = usbd_msc_check_command(common, 10, DATA_DIR_TO_HOST,
					  (0xf<<2) | (1<<8), 1,
					  "SCSI_READ_CAPACITY");
		if (reply == 0)
			reply = usbd_msc_read_capacity(common);
		break;

	case SCSI_READ_FORMAT_CAPACITIES:
		common->data_size_from_cmnd = common->scsi_cmnd[7]<<8 | common->scsi_cmnd[8];
		reply = usbd_msc_check_command(common, 10, DATA_DIR_TO_HOST,
					  (3<<7), 1,
					  "SCSI_READ_FORMAT_CAPACITIES");
		if (reply == 0)
			reply = usbd_msc_read_format_capacities(common);
		break;

	case SCSI_REQUEST_SENSE:
		common->data_size_from_cmnd = common->scsi_cmnd[4];
		reply = usbd_msc_check_command(common, 6, DATA_DIR_TO_HOST,
					  (1<<4), 0,
					  "SCSI_REQUEST_SENSE");
		if (reply == 0)
			reply = usbd_msc_request_sense(common);
		break;

	case SCSI_START_STOP_UNIT:
		common->data_size_from_cmnd = 0;
		reply = usbd_msc_check_command(common, 6, DATA_DIR_NONE,
					  (1<<1) | (1<<4), 0,
					  "SCSI_START-STOP UNIT");
		if (reply == 0)
			reply = usbd_msc_start_stop(common);
		break;

	case SCSI_SYNCHRONIZE_CACHE:
		common->data_size_from_cmnd = 0;
		reply = usbd_msc_check_command(common, 10, DATA_DIR_NONE,
					  (0xf<<2) | (3<<7), 1,
					  "SCSI_SYNCHRONIZE_CACHE");
		if (reply == 0)
			reply = 0;	// we donot have cache
		break;

	case SCSI_TEST_UNIT_READY:
		common->data_size_from_cmnd = 0;
		reply = usbd_msc_check_command(common, 6, DATA_DIR_NONE,
				0, 1,
				"SCSI_TEST_UNIT_READY");
		break;
	/*
	 * Although optional, this command is used by MS-Windows.  We
	 * support a minimal version: BytChk must be 0.
	 */
	case SCSI_VERIFY10:
		common->data_size_from_cmnd = 0;
		reply = usbd_msc_check_command(common, 10, DATA_DIR_NONE,
					  (1<<1) | (0xf<<2) | (3<<7), 1,
					  "SCSI_VERIFY");
		if (reply == 0)
			reply = usbd_msc_verify(common);
		break;

	case SCSI_WRITE6:
		i = common->scsi_cmnd[4];
		common->data_size_from_cmnd = (i == 0) ? 256 : i;
		reply = check_command_size_in_blocks(common, 6,
					  DATA_DIR_FROM_HOST,
					  (7<<1) | (1<<4), 1,
					  "SCSI_WRITE(6)");
		if (reply == 0)
			reply = usbd_msc_write(common);
		break;

	case SCSI_WRITE10:
		common->data_size_from_cmnd = common->scsi_cmnd[7]<<8 | common->scsi_cmnd[8];
		reply = check_command_size_in_blocks(common, 10,
					  DATA_DIR_FROM_HOST,
					  (1<<1) | (0xf<<2) | (3<<7), 1,
					  "SCSI_WRITE(10)");
		if (reply == 0)
			reply = usbd_msc_write(common);
		break;

	case SCSI_WRITE12:
		common->data_size_from_cmnd =
				common->scsi_cmnd[6]<<24 | common->scsi_cmnd[7] << 16 |common->scsi_cmnd[8]<<8|common->scsi_cmnd[9];
		reply = check_command_size_in_blocks(common, 12,
					  DATA_DIR_FROM_HOST,
					  (1<<1) | (0xf<<2) | (0xf<<6), 1,
					  "SCSI_WRITE(12)");
		if (reply == 0)
			reply = usbd_msc_write(common);
		break;

	/*
	 * Some mandatory commands that we recognize but don't implement.
	 * They don't mean much in this setting.  It's left as an exercise
	 * for anyone interested to implement RESERVE and RELEASE in terms
	 * of Posix locks.
	 */
	case SCSI_FORMAT_UNIT:
	//case SCSI_RELEASE:
	//case SCSI_RESERVE:
	case SCSI_SEND_DIAGNOSTIC:
		/* Fall through */

	default:
		common->data_size_from_cmnd = 0;
		sprintf(unknown, "Unknown x%02x", common->scsi_cmnd[0]);
		reply = usbd_msc_check_command(common, common->cmnd_size,
					  DATA_DIR_UNKNOWN, ~0, 0, unknown);
		if (reply == 0) {
			common->curlun->sense_data = SS_INVALID_COMMAND;
			reply = -EINVAL;
		}
		break;

	}

	if (reply == -EINVAL)
		reply = 0;		
	/* Error reply length */	

	/* reply to host */
	if (reply >= 0 && common->data_dir == DATA_DIR_TO_HOST){
		reply = min((u32)reply, common->data_size_from_cmnd);
retry:
		/* if something wrong with the current */
		if(common->curbh == NULL){
			common->curbh = usbd_msc_get_bufhd(common);
			if(common->curbh == NULL){
				rtw_mdelay_os(1);
				goto retry;
			}
		}
		common->curbh->reqin->length = reply;
		common->residue -= reply;
		//USBD_PRINTF("common->curbh submit 0x%X\n",common->curbh);
		if(common->residue){
			common->curbh->reqin->zero= 1;
			if (usbd_msc_bulk_in_transfer(common->mscdev, common->curbh->reqin))
				return -EIO;
			//USBD_PRINTF("usbd_msc_halt_bulk_in_endpoint\n");
			if (common->can_stall)
				usbd_msc_halt_bulk_in_endpoint(common->mscdev);
		}
		else{
			common->curbh->reqin->zero= 0;
			if (usbd_msc_bulk_in_transfer(common->mscdev, common->curbh->reqin))
				return -EIO;
		}
		common->curbh = NULL;
	}
	
	// send back status
	return usbd_msc_scsi_send_status(common);
}

int usbd_msc_receive_cbw(struct msc_dev *mscdev, struct usb_request *req){
	struct bulk_cb_wrap *cbw = req->buf;
	struct msc_common * common = mscdev->common;
	
	/* Is the CBW valid? */
	if (req->actual != US_BULK_CB_WRAP_LEN ||
			cbw->Signature != rtk_cpu_to_le32(
				US_BULK_CB_SIGN)) {
		USBD_ERROR("invalid CBW: len %d sig 0x%x\n",
				req->actual,
				rtk_le32_to_cpu(cbw->Signature));

		/*
		 * The Bulk-only spec says we MUST stall the IN endpoint
		 * (6.6.1), so it's unavoidable.  It also says we must
		 * retain this state until the next reset, but there's
		 * no way to tell the controller driver it should ignore
		 * Clear-Feature(HALT) requests.
		 *
		 * We aren't required to halt the OUT endpoint; instead
		 * we can simply accept and discard any data received
		 * until the next reset.
		 */

		// TODO:
		return -EINVAL;
	}

	/* Is the CBW meaningful? */
	if (cbw->Lun >= MSC_MAX_LUNS ||
		cbw->Flags & ~US_BULK_FLAG_IN ||
			cbw->Length <= 0 || cbw->Length > MAX_COMMAND_SIZE) {
		USBD_ERROR("non-meaningful CBW: lun = %u, flags = 0x%x, "
				"cmdlen %u\n",
				cbw->Lun, cbw->Flags, cbw->Length);

		/*
		 * We can do anything we want here, so let's stall the
		 * bulk pipes if we are allowed to.
		 */

		usb_ep_set_halt(mscdev->out_ep);
		usbd_msc_halt_bulk_in_endpoint(mscdev);
		return -EINVAL;
	}

	// store cbw info in msc_common structure

	common->cmnd_size = cbw->Length;
	rtw_memcpy(common->scsi_cmnd, cbw->CDB, common->cmnd_size);
	if (cbw->Flags & US_BULK_FLAG_IN)
		common->data_dir = DATA_DIR_TO_HOST;
	else
		common->data_dir = DATA_DIR_FROM_HOST;
	common->data_size = rtk_le32_to_cpu(cbw->DataTransferLength);
	if (common->data_size == 0)
		common->data_dir = DATA_DIR_NONE;
	common->lun = cbw->Lun;

	if (common->lun < common->nluns)
		common->curlun = common->luns[common->lun];
	else
		common->curlun = NULL;

	common->tag = cbw->Tag;

	return usbd_msc_scsi_cmd_handler(common);
}


#include "ekm_log.h"
#include "ekm_log_generated.h"
#include "ekm_log_user.h"

void init_ekm_record_names(void)
{
	// set record names
    ekm_register_known_category(EKM_BH_CAT, "bh_*");
    ekm_register_known_type(EKM_BH_dirty, "bh_dirty");
    ekm_register_known_type(EKM_BH_clean, "bh_clean");
    ekm_register_known_type(EKM_BH_lock, "bh_lock");
    ekm_register_known_type(EKM_BH_wait, "bh_wait");
    ekm_register_known_type(EKM_BH_unlock, "bh_unlock");
    ekm_register_known_type(EKM_BH_write, "bh_write");
    ekm_register_known_type(EKM_BH_write_sync, "bh_write_sync");
    ekm_register_known_type(EKM_BH_hash, "bh_hash");
    ekm_register_known_type(EKM_BH_reiser_dirty, "bh_reiser_dirty");
    ekm_register_known_type(EKM_BH_done, "bh_done");
    ekm_register_known_category(EKM_BIO_CAT, "bio_*");
    ekm_register_known_type(EKM_BIO_start_write, "bio_start_write");
    ekm_register_known_type(EKM_BIO_wait, "bio_wait");
    ekm_register_known_type(EKM_BIO_write, "bio_write");
    ekm_register_known_type(EKM_BIO_read, "bio_read");
    ekm_register_known_type(EKM_BIO_write_done, "bio_write_done");

}

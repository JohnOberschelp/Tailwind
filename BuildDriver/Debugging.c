//
//  Tailwind: A File System Driver
//  Copyright (C) 2024 John Oberschelp
//
//  This program is free software: you can redistribute it and/or modify it under the
//  terms of the GNU General Public License as published by the Free Software
//  Foundation, either version 3 of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License along with this
//  program. If not, see https://www.gnu.org/licenses/.
//

#include "Common.h"

static const char HexDigits[] = "0123456789ABCDEF";

//////////////////////////////////////////////////////////////////////

#define PLAIN_THEN_QUOTED( X )  { X, #X }  //  (ex) PLAIN_THEN_QUOTED( foo ) expands to { FOO, "FOO" }

typedef struct _PLAIN_THEN_QUOTED_PAIR
 {
   const ULONG Value;
   const S1_   Quoted;
 } PLAIN_THEN_QUOTED_PAIR;

//////////////////////////////////////////////////////////////////////

//  List of file system minor control codes from ntifs.h

static PLAIN_THEN_QUOTED_PAIR FsCtlMinors[] =
{

    PLAIN_THEN_QUOTED( IRP_MN_USER_FS_REQUEST  ),
    PLAIN_THEN_QUOTED( IRP_MN_MOUNT_VOLUME     ),
    PLAIN_THEN_QUOTED( IRP_MN_VERIFY_VOLUME    ),
    PLAIN_THEN_QUOTED( IRP_MN_LOAD_FILE_SYSTEM ),
    PLAIN_THEN_QUOTED( IRP_MN_TRACK_LINK       ),
    PLAIN_THEN_QUOTED( IRP_MN_KERNEL_CALL      ),
    { 0,0 }
};

//////////////////////////////////////////////////////////////////////

//  List of file system control codes from ntifs.h

static PLAIN_THEN_QUOTED_PAIR FsCtls[] =
{
    PLAIN_THEN_QUOTED( FSCTL_REQUEST_OPLOCK_LEVEL_1   ),
    PLAIN_THEN_QUOTED( FSCTL_REQUEST_OPLOCK_LEVEL_2   ),
    PLAIN_THEN_QUOTED( FSCTL_REQUEST_BATCH_OPLOCK     ),
    PLAIN_THEN_QUOTED( FSCTL_OPLOCK_BREAK_ACKNOWLEDGE ),
    PLAIN_THEN_QUOTED( FSCTL_OPBATCH_ACK_CLOSE_PENDING ),
    PLAIN_THEN_QUOTED( FSCTL_OPLOCK_BREAK_NOTIFY      ),
    PLAIN_THEN_QUOTED( FSCTL_LOCK_VOLUME              ),
    PLAIN_THEN_QUOTED( FSCTL_UNLOCK_VOLUME            ),
    PLAIN_THEN_QUOTED( FSCTL_DISMOUNT_VOLUME          ),
    // decommissioned fsctl value                                              9
    PLAIN_THEN_QUOTED( FSCTL_IS_VOLUME_MOUNTED        ),
    PLAIN_THEN_QUOTED( FSCTL_IS_PATHNAME_VALID        ),
    PLAIN_THEN_QUOTED( FSCTL_MARK_VOLUME_DIRTY        ),
    // decommissioned fsctl value                                             13
    PLAIN_THEN_QUOTED( FSCTL_QUERY_RETRIEVAL_POINTERS ),
    PLAIN_THEN_QUOTED( FSCTL_GET_COMPRESSION          ),
    PLAIN_THEN_QUOTED( FSCTL_SET_COMPRESSION          ),
    // decommissioned fsctl value                                             17
    // decommissioned fsctl value                                             18
    PLAIN_THEN_QUOTED( FSCTL_SET_BOOTLOADER_ACCESSED  ),
    PLAIN_THEN_QUOTED( FSCTL_OPLOCK_BREAK_ACK_NO_2    ),
    PLAIN_THEN_QUOTED( FSCTL_INVALIDATE_VOLUMES       ),
    PLAIN_THEN_QUOTED( FSCTL_QUERY_FAT_BPB            ),
    PLAIN_THEN_QUOTED( FSCTL_REQUEST_FILTER_OPLOCK    ),
    PLAIN_THEN_QUOTED( FSCTL_FILESYSTEM_GET_STATISTICS ),
    PLAIN_THEN_QUOTED( FSCTL_GET_NTFS_VOLUME_DATA     ),
    PLAIN_THEN_QUOTED( FSCTL_GET_NTFS_FILE_RECORD     ),
    PLAIN_THEN_QUOTED( FSCTL_GET_VOLUME_BITMAP        ),
    PLAIN_THEN_QUOTED( FSCTL_GET_RETRIEVAL_POINTERS   ),
    PLAIN_THEN_QUOTED( FSCTL_MOVE_FILE                ),
    PLAIN_THEN_QUOTED( FSCTL_IS_VOLUME_DIRTY          ),
    // decommissioned fsctl value                                             31
    PLAIN_THEN_QUOTED( FSCTL_ALLOW_EXTENDED_DASD_IO   ),
    // decommissioned fsctl value                                             33
    // decommissioned fsctl value                                             34
    PLAIN_THEN_QUOTED( FSCTL_FIND_FILES_BY_SID        ),
    // decommissioned fsctl value                                             36
    // decommissioned fsctl value                                             37
    PLAIN_THEN_QUOTED( FSCTL_SET_OBJECT_ID            ),
    PLAIN_THEN_QUOTED( FSCTL_GET_OBJECT_ID            ),
    PLAIN_THEN_QUOTED( FSCTL_DELETE_OBJECT_ID         ),
    PLAIN_THEN_QUOTED( FSCTL_SET_REPARSE_POINT        ),
    PLAIN_THEN_QUOTED( FSCTL_GET_REPARSE_POINT        ),
    PLAIN_THEN_QUOTED( FSCTL_DELETE_REPARSE_POINT     ),
    PLAIN_THEN_QUOTED( FSCTL_ENUM_USN_DATA            ),
    PLAIN_THEN_QUOTED( FSCTL_SECURITY_ID_CHECK        ),
    PLAIN_THEN_QUOTED( FSCTL_READ_USN_JOURNAL         ),
    PLAIN_THEN_QUOTED( FSCTL_SET_OBJECT_ID_EXTENDED   ),
    PLAIN_THEN_QUOTED( FSCTL_CREATE_OR_GET_OBJECT_ID  ),
    PLAIN_THEN_QUOTED( FSCTL_SET_SPARSE               ),
    PLAIN_THEN_QUOTED( FSCTL_SET_ZERO_DATA            ),
    PLAIN_THEN_QUOTED( FSCTL_QUERY_ALLOCATED_RANGES   ),
    PLAIN_THEN_QUOTED( FSCTL_ENABLE_UPGRADE           ),
    PLAIN_THEN_QUOTED( FSCTL_SET_ENCRYPTION           ),
    PLAIN_THEN_QUOTED( FSCTL_ENCRYPTION_FSCTL_IO      ),
    PLAIN_THEN_QUOTED( FSCTL_WRITE_RAW_ENCRYPTED      ),
    PLAIN_THEN_QUOTED( FSCTL_READ_RAW_ENCRYPTED       ),
    PLAIN_THEN_QUOTED( FSCTL_CREATE_USN_JOURNAL       ),
    PLAIN_THEN_QUOTED( FSCTL_READ_FILE_USN_DATA       ),
    PLAIN_THEN_QUOTED( FSCTL_WRITE_USN_CLOSE_RECORD   ),
    PLAIN_THEN_QUOTED( FSCTL_EXTEND_VOLUME            ),
    PLAIN_THEN_QUOTED( FSCTL_QUERY_USN_JOURNAL        ),
    PLAIN_THEN_QUOTED( FSCTL_DELETE_USN_JOURNAL       ),
    PLAIN_THEN_QUOTED( FSCTL_MARK_HANDLE              ),
    PLAIN_THEN_QUOTED( FSCTL_SIS_COPYFILE             ),
    PLAIN_THEN_QUOTED( FSCTL_SIS_LINK_FILES           ),
    // decommissioned fsctl value                                             66
    // decommissioned fsctl value                                             67
    // decommissioned fsctl value                                             68
    PLAIN_THEN_QUOTED( FSCTL_RECALL_FILE              ),
    // decommissioned fsctl value                                             70
    PLAIN_THEN_QUOTED( FSCTL_READ_FROM_PLEX           ),
    PLAIN_THEN_QUOTED( FSCTL_FILE_PREFETCH            ),
    PLAIN_THEN_QUOTED( FSCTL_MAKE_MEDIA_COMPATIBLE        ),
    PLAIN_THEN_QUOTED( FSCTL_SET_DEFECT_MANAGEMENT        ),
    PLAIN_THEN_QUOTED( FSCTL_QUERY_SPARING_INFO           ),
    PLAIN_THEN_QUOTED( FSCTL_QUERY_ON_DISK_VOLUME_INFO    ),
    PLAIN_THEN_QUOTED( FSCTL_SET_VOLUME_COMPRESSION_STATE ),
    // decommissioned fsctl value                                                 80
    PLAIN_THEN_QUOTED( FSCTL_TXFS_MODIFY_RM               ),
    PLAIN_THEN_QUOTED( FSCTL_TXFS_QUERY_RM_INFORMATION    ),
    // decommissioned fsctl value                                                 83
    PLAIN_THEN_QUOTED( FSCTL_TXFS_ROLLFORWARD_REDO        ),
    PLAIN_THEN_QUOTED( FSCTL_TXFS_ROLLFORWARD_UNDO        ),
    PLAIN_THEN_QUOTED( FSCTL_TXFS_START_RM                ),
    PLAIN_THEN_QUOTED( FSCTL_TXFS_SHUTDOWN_RM             ),
    PLAIN_THEN_QUOTED( FSCTL_TXFS_READ_BACKUP_INFORMATION ),
    PLAIN_THEN_QUOTED( FSCTL_TXFS_WRITE_BACKUP_INFORMATION ),
    PLAIN_THEN_QUOTED( FSCTL_TXFS_CREATE_SECONDARY_RM     ),
    PLAIN_THEN_QUOTED( FSCTL_TXFS_GET_METADATA_INFO       ),
    PLAIN_THEN_QUOTED( FSCTL_TXFS_GET_TRANSACTED_VERSION  ),
    // decommissioned fsctl value                                                 93
    PLAIN_THEN_QUOTED( FSCTL_TXFS_SAVEPOINT_INFORMATION   ),
    PLAIN_THEN_QUOTED( FSCTL_TXFS_CREATE_MINIVERSION      ),
    // decommissioned fsctl value                                                 96
    // decommissioned fsctl value                                                 97
    // decommissioned fsctl value                                                 98
    PLAIN_THEN_QUOTED( FSCTL_TXFS_TRANSACTION_ACTIVE      ),
    PLAIN_THEN_QUOTED( FSCTL_SET_ZERO_ON_DEALLOCATION     ),
    PLAIN_THEN_QUOTED( FSCTL_SET_REPAIR                   ),
    PLAIN_THEN_QUOTED( FSCTL_GET_REPAIR                   ),
    PLAIN_THEN_QUOTED( FSCTL_WAIT_FOR_REPAIR              ),
    // decommissioned fsctl value                                                 105
    PLAIN_THEN_QUOTED( FSCTL_INITIATE_REPAIR              ),
    PLAIN_THEN_QUOTED( FSCTL_CSC_INTERNAL                 ),
    PLAIN_THEN_QUOTED( FSCTL_SHRINK_VOLUME                ),
    PLAIN_THEN_QUOTED( FSCTL_SET_SHORT_NAME_BEHAVIOR      ),
    PLAIN_THEN_QUOTED( FSCTL_DFSR_SET_GHOST_HANDLE_STATE  ),
    PLAIN_THEN_QUOTED( FSCTL_TXFS_LIST_TRANSACTIONS       ),
    PLAIN_THEN_QUOTED( FSCTL_QUERY_PAGEFILE_ENCRYPTION    ),
    PLAIN_THEN_QUOTED( FSCTL_RESET_VOLUME_ALLOCATION_HINTS ),
    PLAIN_THEN_QUOTED( FSCTL_QUERY_DEPENDENT_VOLUME       ),
    PLAIN_THEN_QUOTED( FSCTL_SD_GLOBAL_CHANGE             ),
    PLAIN_THEN_QUOTED( FSCTL_TXFS_READ_BACKUP_INFORMATION2 ),
    PLAIN_THEN_QUOTED( FSCTL_LOOKUP_STREAM_FROM_CLUSTER   ),
    PLAIN_THEN_QUOTED( FSCTL_TXFS_WRITE_BACKUP_INFORMATION2 ),
    PLAIN_THEN_QUOTED( FSCTL_FILE_TYPE_NOTIFICATION       ),
    PLAIN_THEN_QUOTED( FSCTL_FILE_LEVEL_TRIM              ),
    PLAIN_THEN_QUOTED( FSCTL_GET_BOOT_AREA_INFO           ),
    PLAIN_THEN_QUOTED( FSCTL_GET_RETRIEVAL_POINTER_BASE   ),
    PLAIN_THEN_QUOTED( FSCTL_SET_PERSISTENT_VOLUME_STATE  ),
    PLAIN_THEN_QUOTED( FSCTL_QUERY_PERSISTENT_VOLUME_STATE ),
    PLAIN_THEN_QUOTED( FSCTL_REQUEST_OPLOCK               ),
    PLAIN_THEN_QUOTED( FSCTL_CSV_TUNNEL_REQUEST           ),
    PLAIN_THEN_QUOTED( FSCTL_IS_CSV_FILE                  ),
    PLAIN_THEN_QUOTED( FSCTL_QUERY_FILE_SYSTEM_RECOGNITION ),
    PLAIN_THEN_QUOTED( FSCTL_CSV_GET_VOLUME_PATH_NAME     ),
    PLAIN_THEN_QUOTED( FSCTL_CSV_GET_VOLUME_NAME_FOR_VOLUME_MOUNT_POINT ),
    PLAIN_THEN_QUOTED( FSCTL_CSV_GET_VOLUME_PATH_NAMES_FOR_VOLUME_NAME  ),
    PLAIN_THEN_QUOTED( FSCTL_IS_FILE_ON_CSV_VOLUME        ),
    PLAIN_THEN_QUOTED( FSCTL_CORRUPTION_HANDLING          ),
    PLAIN_THEN_QUOTED( FSCTL_OFFLOAD_READ                 ),
    PLAIN_THEN_QUOTED( FSCTL_OFFLOAD_WRITE                ),
    PLAIN_THEN_QUOTED( FSCTL_CSV_INTERNAL                 ),
    PLAIN_THEN_QUOTED( FSCTL_SET_PURGE_FAILURE_MODE       ),
    PLAIN_THEN_QUOTED( FSCTL_QUERY_FILE_LAYOUT            ),
    PLAIN_THEN_QUOTED( FSCTL_IS_VOLUME_OWNED_BYCSVFS      ),
    PLAIN_THEN_QUOTED( FSCTL_GET_INTEGRITY_INFORMATION    ),
    PLAIN_THEN_QUOTED( FSCTL_SET_INTEGRITY_INFORMATION    ),
    PLAIN_THEN_QUOTED( FSCTL_QUERY_FILE_REGIONS           ),
    PLAIN_THEN_QUOTED( FSCTL_RKF_INTERNAL                 ),
    PLAIN_THEN_QUOTED( FSCTL_SCRUB_DATA                   ),
    PLAIN_THEN_QUOTED( FSCTL_REPAIR_COPIES                ),
    PLAIN_THEN_QUOTED( FSCTL_DISABLE_LOCAL_BUFFERING      ),
    PLAIN_THEN_QUOTED( FSCTL_CSV_MGMT_LOCK                ),
    PLAIN_THEN_QUOTED( FSCTL_CSV_QUERY_DOWN_LEVEL_FILE_SYSTEM_CHARACTERISTICS ),
    PLAIN_THEN_QUOTED( FSCTL_ADVANCE_FILE_ID              ),
    PLAIN_THEN_QUOTED( FSCTL_CSV_SYNC_TUNNEL_REQUEST      ),
    PLAIN_THEN_QUOTED( FSCTL_CSV_QUERY_VETO_FILE_DIRECT_IO),
    PLAIN_THEN_QUOTED( FSCTL_WRITE_USN_REASON             ),
    PLAIN_THEN_QUOTED( FSCTL_CSV_CONTROL                  ),
    PLAIN_THEN_QUOTED( FSCTL_GET_REFS_VOLUME_DATA         ),
    PLAIN_THEN_QUOTED( FSCTL_CSV_H_BREAKING_SYNC_TUNNEL_REQUEST ),
    PLAIN_THEN_QUOTED( FSCTL_QUERY_STORAGE_CLASSES        ),
    PLAIN_THEN_QUOTED( FSCTL_QUERY_REGION_INFO            ),
    PLAIN_THEN_QUOTED( FSCTL_USN_TRACK_MODIFIED_RANGES    ),
    PLAIN_THEN_QUOTED( FSCTL_QUERY_SHARED_VIRTUAL_DISK_SUPPORT ),
    PLAIN_THEN_QUOTED( FSCTL_SVHDX_SYNC_TUNNEL_REQUEST         ),
    PLAIN_THEN_QUOTED( FSCTL_SVHDX_SET_INITIATOR_INFORMATION   ),
    PLAIN_THEN_QUOTED( FSCTL_SET_EXTERNAL_BACKING              ),
    PLAIN_THEN_QUOTED( FSCTL_GET_EXTERNAL_BACKING              ),
    PLAIN_THEN_QUOTED( FSCTL_DELETE_EXTERNAL_BACKING           ),
    PLAIN_THEN_QUOTED( FSCTL_ENUM_EXTERNAL_BACKING             ),
    PLAIN_THEN_QUOTED( FSCTL_ENUM_OVERLAY                      ),
    PLAIN_THEN_QUOTED( FSCTL_ADD_OVERLAY                       ),
    PLAIN_THEN_QUOTED( FSCTL_REMOVE_OVERLAY                    ),
    PLAIN_THEN_QUOTED( FSCTL_UPDATE_OVERLAY                    ),
    PLAIN_THEN_QUOTED( FSCTL_SHUFFLE_FILE                      ),
    PLAIN_THEN_QUOTED( FSCTL_DUPLICATE_EXTENTS_TO_FILE         ),
    PLAIN_THEN_QUOTED( FSCTL_SPARSE_OVERALLOCATE               ),
    PLAIN_THEN_QUOTED( FSCTL_STORAGE_QOS_CONTROL               ),
    PLAIN_THEN_QUOTED( FSCTL_INITIATE_FILE_METADATA_OPTIMIZATION      ),
    PLAIN_THEN_QUOTED( FSCTL_QUERY_FILE_METADATA_OPTIMIZATION         ),
    PLAIN_THEN_QUOTED( FSCTL_SVHDX_ASYNC_TUNNEL_REQUEST        ),
    PLAIN_THEN_QUOTED( FSCTL_GET_WOF_VERSION                   ),
    PLAIN_THEN_QUOTED( FSCTL_HCS_SYNC_TUNNEL_REQUEST           ),
    PLAIN_THEN_QUOTED( FSCTL_HCS_ASYNC_TUNNEL_REQUEST          ),
    PLAIN_THEN_QUOTED( FSCTL_QUERY_EXTENT_READ_CACHE_INFO      ),
    PLAIN_THEN_QUOTED( FSCTL_QUERY_REFS_VOLUME_COUNTER_INFO    ),
    PLAIN_THEN_QUOTED( FSCTL_CLEAN_VOLUME_METADATA             ),
    PLAIN_THEN_QUOTED( FSCTL_SET_INTEGRITY_INFORMATION_EX      ),
    PLAIN_THEN_QUOTED( FSCTL_SUSPEND_OVERLAY                   ),
    PLAIN_THEN_QUOTED( FSCTL_VIRTUAL_STORAGE_QUERY_PROPERTY    ),
    PLAIN_THEN_QUOTED( FSCTL_FILESYSTEM_GET_STATISTICS_EX      ),
    PLAIN_THEN_QUOTED( FSCTL_QUERY_VOLUME_CONTAINER_STATE      ),
    PLAIN_THEN_QUOTED( FSCTL_SET_LAYER_ROOT                    ),
    PLAIN_THEN_QUOTED( FSCTL_QUERY_DIRECT_ACCESS_EXTENTS       ),
    PLAIN_THEN_QUOTED( FSCTL_NOTIFY_STORAGE_SPACE_ALLOCATION   ),
    PLAIN_THEN_QUOTED( FSCTL_SSDI_STORAGE_REQUEST              ),
    PLAIN_THEN_QUOTED( FSCTL_QUERY_DIRECT_IMAGE_ORIGINAL_BASE  ),
    PLAIN_THEN_QUOTED( FSCTL_READ_UNPRIVILEGED_USN_JOURNAL     ),
    PLAIN_THEN_QUOTED( FSCTL_GHOST_FILE_EXTENTS                ),
    PLAIN_THEN_QUOTED( FSCTL_QUERY_GHOSTED_FILE_EXTENTS        ),
    PLAIN_THEN_QUOTED( FSCTL_UNMAP_SPACE                       ),
    PLAIN_THEN_QUOTED( FSCTL_HCS_SYNC_NO_WRITE_TUNNEL_REQUEST  ),
    PLAIN_THEN_QUOTED( FSCTL_START_VIRTUALIZATION_INSTANCE     ),
    PLAIN_THEN_QUOTED( FSCTL_GET_FILTER_FILE_IDENTIFIER        ),
    PLAIN_THEN_QUOTED( FSCTL_STREAMS_QUERY_PARAMETERS          ),
    PLAIN_THEN_QUOTED( FSCTL_STREAMS_ASSOCIATE_ID              ),
    PLAIN_THEN_QUOTED( FSCTL_STREAMS_QUERY_ID                  ),
    PLAIN_THEN_QUOTED( FSCTL_GET_RETRIEVAL_POINTERS_AND_REFCOUNT ),
    PLAIN_THEN_QUOTED( FSCTL_QUERY_VOLUME_NUMA_INFO              ),
    PLAIN_THEN_QUOTED( FSCTL_REFS_DEALLOCATE_RANGES              ),
    PLAIN_THEN_QUOTED( FSCTL_QUERY_REFS_SMR_VOLUME_INFO          ),
    PLAIN_THEN_QUOTED( FSCTL_SET_REFS_SMR_VOLUME_GC_PARAMETERS   ),
    PLAIN_THEN_QUOTED( FSCTL_SET_REFS_FILE_STRICTLY_SEQUENTIAL   ),
    PLAIN_THEN_QUOTED( FSCTL_DUPLICATE_EXTENTS_TO_FILE_EX        ),
    PLAIN_THEN_QUOTED( FSCTL_QUERY_BAD_RANGES                    ),
    PLAIN_THEN_QUOTED( FSCTL_SET_DAX_ALLOC_ALIGNMENT_HINT        ),
    PLAIN_THEN_QUOTED( FSCTL_DELETE_CORRUPTED_REFS_CONTAINER     ),
    PLAIN_THEN_QUOTED( FSCTL_SCRUB_UNDISCOVERABLE_ID             ),
    PLAIN_THEN_QUOTED( FSCTL_NOTIFY_DATA_CHANGE                  ),
    PLAIN_THEN_QUOTED( FSCTL_START_VIRTUALIZATION_INSTANCE_EX    ),
    PLAIN_THEN_QUOTED( FSCTL_ENCRYPTION_KEY_CONTROL              ),
    PLAIN_THEN_QUOTED( FSCTL_VIRTUAL_STORAGE_SET_BEHAVIOR        ),
    PLAIN_THEN_QUOTED( FSCTL_SET_REPARSE_POINT_EX                ),
    PLAIN_THEN_QUOTED( FSCTL_REARRANGE_FILE                      ),
    PLAIN_THEN_QUOTED( FSCTL_VIRTUAL_STORAGE_PASSTHROUGH         ),
    PLAIN_THEN_QUOTED( FSCTL_GET_RETRIEVAL_POINTER_COUNT         ),
    PLAIN_THEN_QUOTED( FSCTL_ENABLE_PER_IO_FLAGS                 ),
    PLAIN_THEN_QUOTED( FSCTL_LMR_GET_LINK_TRACKING_INFORMATION   ),
    PLAIN_THEN_QUOTED( FSCTL_LMR_SET_LINK_TRACKING_INFORMATION   ),
    PLAIN_THEN_QUOTED( IOCTL_LMR_ARE_FILE_OBJECTS_ON_SAME_SERVER ),
    PLAIN_THEN_QUOTED( FSCTL_PIPE_ASSIGN_EVENT             ),
    PLAIN_THEN_QUOTED( FSCTL_PIPE_DISCONNECT               ),
    PLAIN_THEN_QUOTED( FSCTL_PIPE_LISTEN                   ),
    PLAIN_THEN_QUOTED( FSCTL_PIPE_PEEK                     ),
    PLAIN_THEN_QUOTED( FSCTL_PIPE_QUERY_EVENT              ),
    PLAIN_THEN_QUOTED( FSCTL_PIPE_TRANSCEIVE               ),
    PLAIN_THEN_QUOTED( FSCTL_PIPE_WAIT                     ),
    PLAIN_THEN_QUOTED( FSCTL_PIPE_IMPERSONATE              ),
    PLAIN_THEN_QUOTED( FSCTL_PIPE_SET_CLIENT_PROCESS       ),
    PLAIN_THEN_QUOTED( FSCTL_PIPE_QUERY_CLIENT_PROCESS     ),
    PLAIN_THEN_QUOTED( FSCTL_PIPE_GET_PIPE_ATTRIBUTE       ),
    PLAIN_THEN_QUOTED( FSCTL_PIPE_SET_PIPE_ATTRIBUTE       ),
    PLAIN_THEN_QUOTED( FSCTL_PIPE_GET_CONNECTION_ATTRIBUTE ),
    PLAIN_THEN_QUOTED( FSCTL_PIPE_SET_CONNECTION_ATTRIBUTE ),
    PLAIN_THEN_QUOTED( FSCTL_PIPE_GET_HANDLE_ATTRIBUTE     ),
    PLAIN_THEN_QUOTED( FSCTL_PIPE_SET_HANDLE_ATTRIBUTE     ),
    PLAIN_THEN_QUOTED( FSCTL_PIPE_FLUSH                    ),
    PLAIN_THEN_QUOTED( FSCTL_PIPE_DISABLE_IMPERSONATE      ),
    PLAIN_THEN_QUOTED( FSCTL_PIPE_SILO_ARRIVAL             ),
    PLAIN_THEN_QUOTED( FSCTL_PIPE_CREATE_SYMLINK           ),
    PLAIN_THEN_QUOTED( FSCTL_PIPE_DELETE_SYMLINK           ),
    PLAIN_THEN_QUOTED( FSCTL_PIPE_QUERY_CLIENT_PROCESS_V2  ),
    PLAIN_THEN_QUOTED( FSCTL_PIPE_INTERNAL_READ        ),
    PLAIN_THEN_QUOTED( FSCTL_PIPE_INTERNAL_WRITE       ),
    PLAIN_THEN_QUOTED( FSCTL_PIPE_INTERNAL_TRANSCEIVE  ),
    PLAIN_THEN_QUOTED( FSCTL_PIPE_INTERNAL_READ_OVFLOW ),
    { 0,0 }
};

//////////////////////////////////////////////////////////////////////

const char* GetLongAsHexText( ULONG v )

{
    static char HexToCharBuffer[] = "0x12345678\0";

    HexToCharBuffer[2] = HexDigits[( v >> 28 ) & 0xF];
    HexToCharBuffer[3] = HexDigits[( v >> 24 ) & 0xF];
    HexToCharBuffer[4] = HexDigits[( v >> 20 ) & 0xF];
    HexToCharBuffer[5] = HexDigits[( v >> 16 ) & 0xF];
    HexToCharBuffer[6] = HexDigits[( v >> 12 ) & 0xF];
    HexToCharBuffer[7] = HexDigits[( v >>  8 ) & 0xF];
    HexToCharBuffer[8] = HexDigits[( v >>  4 ) & 0xF];
    HexToCharBuffer[9] = HexDigits[ v        & 0xF];
    return HexToCharBuffer;
}

//////////////////////////////////////////////////////////////////////

const char* GetIrqlAsText()

{
    KIRQL Irql = KeGetCurrentIrql();

    switch ( Irql )
    {
      case PASSIVE_LEVEL  : return "PASSIVE_LEVEL,LOW_LEVEL";          //  0  Passive release level,Lowest interrupt level
      case APC_LEVEL      : return "APC_LEVEL";                        //  1  APC interrupt level
      case DISPATCH_LEVEL : return "DISPATCH_LEVEL";                   //  2  Dispatcher level
      case CMCI_LEVEL     : return "CMCI_LEVEL";                       //  5  CMCI handler level
      case CLOCK_LEVEL    : return "CLOCK_LEVEL";                      // 13  Interval clock level
      case IPI_LEVEL      : return "IPI_LEVEL,DRS_LEVEL,POWER_LEVEL";  // 14  Interprocessor interrupt level,Deferred Recovery Service level,Power failure level
      case PROFILE_LEVEL  : return "PROFILE_LEVEL,HIGH_LEVEL";         // 15  timer used for profiling,Highest interrupt level
    }

    static char HexToCharBuffer[] = "0x00\0";
    HexToCharBuffer[2] = HexDigits[Irql >> 4];
    HexToCharBuffer[3] = HexDigits[Irql & 0xF];
    return HexToCharBuffer;
}

//////////////////////////////////////////////////////////////////////

const char* GetFileInformationClassAsText( FILE_INFORMATION_CLASS FileInformationClass )

{
    switch ( FileInformationClass )
    {
      case FileDirectoryInformation:       return "FILE_DIRECTORY_INFORMATION";    //   1
      case FileFullDirectoryInformation:   return "FILE_FULL_DIR_INFORMATION";     //   2
      case FileIdFullDirectoryInformation: return "FILE_ID_FULL_DIR_INFORMATION";  //  38
      case FileBothDirectoryInformation:   return "FILE_BOTH_DIR_INFORMATION";     //   3
      case FileIdBothDirectoryInformation: return "FILE_ID_BOTH_DIR_INFORMATION";  //  37
      case FileNamesInformation:           return "FILE_NAMES_INFORMATION";        //  12
    }
//BreakToDebugger();
    return GetLongAsHexText( ( long )FileInformationClass );
}

//////////////////////////////////////////////////////////////////////

//  Many values at https://gist.github.com/mattifestation/844d01bd5c378f9b1f52d76219deaf0f
#define IOCTL_SCSI_PASS_THROUGH              0x0004D004
#define IOCTL_MOUNTDEV_QUERY_DEVICE_NAME     0x004D0008
#ifndef IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS
//#define IOCTL_DISK_GET_DRIVE_GEOMETRY        0x00070000
//#define IOCTL_DISK_IS_WRITABLE               0x00070024
//#define IOCTL_DISK_GET_PARTITION_INFO_EX     0x00070048
//#define IOCTL_STORAGE_GET_HOTPLUG_INFO       0x002D0C14
//#define IOCTL_STORAGE_GET_DEVICE_NUMBER      0x002D1080
//#define IOCTL_STORAGE_QUERY_PROPERTY         0x002D1400
#define IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS 0x00560000
#define IOCTL_VOLUME_ONLINE                  0x0056c008
#define IOCTL_VOLUME_OFFLINE                 0x0056c00c
#define IOCTL_VOLUME_IS_CLUSTERED            0x00560030
#define IOCTL_VOLUME_GET_GPT_ATTRIBUTES      0x00560038
#endif

const char* GetIoCtlAsText( ULONG IoCtl )

{
    switch ( IoCtl )
    {
      case IOCTL_SCSI_PASS_THROUGH              : return "IOCTL_SCSI_PASS_THROUGH";
      case IOCTL_DISK_GET_DRIVE_GEOMETRY        : return "IOCTL_DISK_GET_DRIVE_GEOMETRY";
      case IOCTL_DISK_IS_WRITABLE               : return "IOCTL_DISK_IS_WRITABLE";
      case IOCTL_DISK_GET_PARTITION_INFO_EX     : return "IOCTL_DISK_GET_PARTITION_INFO_EX";
      case IOCTL_STORAGE_GET_HOTPLUG_INFO       : return "IOCTL_STORAGE_GET_HOTPLUG_INFO";
      case IOCTL_STORAGE_GET_DEVICE_NUMBER      : return "IOCTL_STORAGE_GET_DEVICE_NUMBER";
      case IOCTL_STORAGE_QUERY_PROPERTY         : return "IOCTL_STORAGE_QUERY_PROPERTY";
      case IOCTL_MOUNTDEV_QUERY_DEVICE_NAME     : return "IOCTL_MOUNTDEV_QUERY_DEVICE_NAME";
      case IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS : return "IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS";
      case IOCTL_VOLUME_ONLINE                  : return "IOCTL_VOLUME_ONLINE";
      case IOCTL_VOLUME_OFFLINE                 : return "IOCTL_VOLUME_OFFLINE";
      case IOCTL_VOLUME_IS_CLUSTERED            : return "IOCTL_VOLUME_IS_CLUSTERED";
      case IOCTL_VOLUME_GET_GPT_ATTRIBUTES      : return "IOCTL_VOLUME_GET_GPT_ATTRIBUTES";
    }

    return GetLongAsHexText( IoCtl );
}

//////////////////////////////////////////////////////////////////////

const char* GetFsControlCodeMinorAsText( ULONG FsControlCodeMinor, S1_ SuppliedBuffer )

{
    for ( int i = 0; ; i++ )
    {
        ULONG Value = FsCtlMinors[i].Value;
        if ( ! Value ) break;
        if ( Value == FsControlCodeMinor )
        {
            strcpy( SuppliedBuffer, FsCtlMinors[i].Quoted );
            return SuppliedBuffer;
        }
    }
    strcpy( SuppliedBuffer, "???" );
    return SuppliedBuffer;
}

//////////////////////////////////////////////////////////////////////

const char* GetFsControlCodeAsText( ULONG FsControlCode, S1_ SuppliedBuffer )

{
    for ( int i = 0; ; i++ )
    {
        ULONG Value = FsCtls[i].Value;
        if ( ! Value ) break;
        if ( Value == FsControlCode )
        {
            strcpy( SuppliedBuffer, FsCtls[i].Quoted );
            return SuppliedBuffer;
        }
    }
    strcpy( SuppliedBuffer, "???" );
    return SuppliedBuffer;
}

//////////////////////////////////////////////////////////////////////

const char* GetAttributesAsText( ULONG Attributes )

{
    static char Buffer[512];
    strcpy( Buffer, GetLongAsHexText( Attributes ) );
    strcat( Buffer, " ( " );
    if ( Attributes &     0x00000001 ) strcat( Buffer, " FILE_ATTRIBUTE_READONLY " );
    if ( Attributes &     0x00000002 ) strcat( Buffer, " FILE_ATTRIBUTE_HIDDEN " );
    if ( Attributes &     0x00000004 ) strcat( Buffer, " FILE_ATTRIBUTE_SYSTEM " );
    if ( Attributes &     0x00000010 ) strcat( Buffer, " FILE_ATTRIBUTE_DIRECTORY " );
    if ( Attributes &     0x00000020 ) strcat( Buffer, " FILE_ATTRIBUTE_ARCHIVE " );
    if ( Attributes &     0x00000040 ) strcat( Buffer, " FILE_ATTRIBUTE_DEVICE " );
    if ( Attributes &     0x00000080 ) strcat( Buffer, " FILE_ATTRIBUTE_NORMAL " );
    if ( Attributes &     0x00000100 ) strcat( Buffer, " FILE_ATTRIBUTE_TEMPORARY " );
    strcat( Buffer, " )" );

    return Buffer;
}

//////////////////////////////////////////////////////////////////////

/*
CreateDisposition
-----------------
Specifies what to do, depending on whether the file already exists, as one of the following values.
    FILE_SUPERSEDE      If the file already exists, replace it with the given file. If it does not, create the given file.
    FILE_CREATE         If the file already exists, fail the request and do not create or open the given file. If it does not, create the given file.
    FILE_OPEN           If the file already exists, open it. If it does not, fail the request and do not create a new file.
    FILE_OPEN_IF        If the file already exists, open it. If it does not, create the given file.
    FILE_OVERWRITE      If the file already exists, open it and overwrite it. If it does not, fail the request.
    FILE_OVERWRITE_IF   If the file already exists, open it and overwrite it. If it does not, create the given file.
*/
const char* GetDispositionAsText( U1 Disposition )

{
    switch ( Disposition )
    {
      case FILE_SUPERSEDE     : return "FILE_SUPERSEDE";     //  0
      case FILE_OPEN          : return "FILE_OPEN";          //  1
      case FILE_CREATE        : return "FILE_CREATE";        //  2
      case FILE_OPEN_IF       : return "FILE_OPEN_IF";       //  3
      case FILE_OVERWRITE     : return "FILE_OVERWRITE";     //  4
      case FILE_OVERWRITE_IF  : return "FILE_OVERWRITE_IF";  //  5
    }
    static char HexToCharBuffer[] = "0x00\0";
    HexToCharBuffer[2] = HexDigits[Disposition >> 4];
    HexToCharBuffer[3] = HexDigits[Disposition & 0xF];
    return HexToCharBuffer;
}

//////////////////////////////////////////////////////////////////////

const char* GetMajorFunctionName( UCHAR MajorFunction )

{
    static const int NumMajorFunctionNamesKnown = 28;
    static const char* MajorFunctionNames [29] = {
        "CREATE","CREATE_NAMED_PIPE","CLOSE","READ","WRITE","QUERY_INFORMATION","SET_INFORMATION",
        "QUERY_EA","SET_EA","FLUSH_BUFFERS","QUERY_VOLUME_INFORMATION","SET_VOLUME_INFORMATION",
        "DIRECTORY_CONTROL","FILE_SYSTEM_CONTROL","DEVICE_CONTROL","INTERNAL_DEVICE_CONTROL",
        "SHUTDOWN","LOCK_CONTROL","CLEANUP","CREATE_MAILSLOT","QUERY_SECURITY","SET_SECURITY",
        "POWER","SYSTEM_CONTROL","DEVICE_CHANGE","QUERY_QUOTA","SET_QUOTA","PNP","unknown"};

    return MajorFunctionNames[min( MajorFunction, NumMajorFunctionNamesKnown )];
}

//////////////////////////////////////////////////////////////////////

const char* GetStatusAsText( NTSTATUS Status )

{
    switch ( Status )
    {

//     "successful" status codes - high two bits are 00    high byte hex 00 thru 3F
      case STATUS_SUCCESS:                 return "STATUS_SUCCESS ok";                 //  0x00000000
      case 0x00000024:                     return "STATUS_BLUE_SCREEN_STOP ok";        //  0x00000024
      case STATUS_ABANDONED:               return "STATUS_ABANDONED ok";               //  0x00000080  The caller attempted to wait for a mutex that has been abandoned.
///////////STATUS_ABANDONED_WAIT_0                                                     //  0x00000080  The caller attempted to wait for a mutex that has been abandoned.
///////////STATUS_ABANDONED_WAIT_63                                                    //  0x000000BF  The caller attempted to wait for a mutex that has been abandoned.
      case STATUS_USER_APC:                return "STATUS_USER_APC ok";                //  0x000000C0  The wait was interrupted to deliver a user APC to the calling thread.
      case STATUS_ALERTED:                 return "STATUS_ALERTED ok";                 //  0x00000101  The delay completed because the thread was alerted.
      case STATUS_TIMEOUT:                 return "STATUS_TIMEOUT ok";                 //  0x00000102  A time-out occurred before the object was set to a signaled state.
      case STATUS_PENDING:                 return "STATUS_PENDING ok";                 //  0x00000103  The operation that was requested is pending completion.

//     "informational" status codes - high two bits are 01   high byte 40 thru 7f

//     "warning" status codes - high two bits are 10   high byte 80 thru bf
      case STATUS_BREAKPOINT:              return "STATUS_BREAKPOINT warn";              //  0x80000003
      case STATUS_SINGLE_STEP:             return "STATUS_SINGLE_STEP warn";             //  0x80000004
      case STATUS_BUFFER_OVERFLOW:         return "STATUS_BUFFER_OVERFLOW warn";         //  0x80000005
      case STATUS_NO_MORE_FILES:           return "STATUS_NO_MORE_FILES warn";           //  0x80000006
      case STATUS_VERIFY_REQUIRED:         return "STATUS_VERIFY_REQUIRED warn";         //  0x80000016

//     "error" status codes - high two bits are 11   high byte c0 thru ff
      case STATUS_UNSUCCESSFUL:            return "STATUS_UNSUCCESSFUL error";            //  0xC0000001  {Operation Failed} The requested operation was unsuccessful.
      case STATUS_NOT_IMPLEMENTED:         return "STATUS_NOT_IMPLEMENTED error";         //  0xC0000002  {Not Implemented} The requested operation is not implemented.
      case 0xC0000003:                     return "STATUS_INVALID_INFO_CLASS error";      //  0xC0000003  {Invalid Parameter} The specified information class is not a valid information class for the specified object.
      case STATUS_ACCESS_VIOLATION:        return "STATUS_ACCESS_VIOLATION error";        //  0xC0000005
      case STATUS_INVALID_PARAMETER:       return "STATUS_INVALID_PARAMETER error";       //  0xC000000D
      case STATUS_NO_SUCH_FILE:            return "STATUS_NO_SUCH_FILE error";            //  0xC000000F
      case STATUS_INVALID_DEVICE_REQUEST:  return "STATUS_INVALID_DEVICE_REQUEST error";  //  0xC0000010
      case STATUS_END_OF_FILE:             return "STATUS_END_OF_FILE error";             //  0xC0000011
    case STATUS_MORE_PROCESSING_REQUIRED:  return "STATUS_MORE_PROCESSING_REQUIRED";      //  0xC0000016
      case STATUS_ACCESS_DENIED:           return "STATUS_ACCESS_DENIED error";           //  0xC0000022
      case STATUS_OBJECT_NAME_NOT_FOUND:   return "STATUS_OBJECT_NAME_NOT_FOUND error";   //  0xC0000034
      case STATUS_OBJECT_NAME_COLLISION:   return "STATUS_OBJECT_NAME_COLLISION error";   //  0xC0000035
      case STATUS_OBJECT_PATH_NOT_FOUND:   return "STATUS_OBJECT_PATH_NOT_FOUND error";   //  0xC000003A
      case STATUS_LOCK_NOT_GRANTED:        return "STATUS_LOCK_NOT_GRANTED error";        //  0xC0000055
      case STATUS_RANGE_NOT_LOCKED:        return "STATUS_RANGE_NOT_LOCKED error";        //  0xC000007E  The range specified in NtUnlockFile was not locked.
      case STATUS_INSUFFICIENT_RESOURCES:  return "STATUS_INSUFFICIENT_RESOURCES error";  //  0xC000009A
      case STATUS_FILE_IS_A_DIRECTORY:     return "STATUS_FILE_IS_A_DIRECTORY error";     //  0xC00000BA
      case STATUS_DIRECTORY_NOT_EMPTY:     return "STATUS_DIRECTORY_NOT_EMPTY error";     //  0xC0000101
      case STATUS_NOT_A_DIRECTORY:         return "STATUS_NOT_A_DIRECTORY error";         //  0xC0000103
      case STATUS_CANNOT_DELETE:           return "STATUS_CANNOT_DELETE";                 //   0xC000121
      case STATUS_UNRECOGNIZED_VOLUME:     return "STATUS_UNRECOGNIZED_VOLUME error";     //  0xC000014F
    }

    return "STATUS_( unknown )";
}

//////////////////////////////////////////////////////////////////////

void DumpRam( const void* AddressAsVoidPointer, int NumBytes )

{
    U1_ Address = ( U1_ )AddressAsVoidPointer;
    while ( NumBytes > 0 )
    {
        //               "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................";
        char  Output[] = "                                                                 ";
        for ( int Column = 0; Column < min( 16, NumBytes ); Column++ )
        {
            U1 Byte = Address[Column];
            Output[3*Column  ] = HexDigits[Byte>>4];
            Output[3*Column+1] = HexDigits[Byte&0xf];
            Output[3*16 + 1 + Column] = ( Byte >= ' ' && Byte <= '~' ) ? Byte : '.';
        }

LogFormatted( "  ( dump )  %p : %s \n", Address, Output );

        NumBytes -= 16;
        Address  += 16;
    }
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


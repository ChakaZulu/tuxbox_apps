#ifndef __enigma_dyn_mount_h
#define __enigma_dyn_mount_h
eString addMountPoint(eString request, eString dirpath, eString opts, eHTTPConnection *content);
eString addMountPointWindow(eString request, eString dirpath, eString opts, eHTTPConnection *content);
eString editMountPoint(eString request, eString dirpath, eString opts, eHTTPConnection *content);
eString editMountPointWindow(eString request, eString dirpath, eString opts, eHTTPConnection *content);
eString removeMountPoint(eString request, eString dirpath, eString opts, eHTTPConnection *content);
eString showMountPoints(eString request, eString dirpath, eString opts, eHTTPConnection *content);
eString mountMountPoint(eString request, eString dirpath, eString opts, eHTTPConnection *content);
eString unmountMountPoint(eString request, eString dirpath, eString opts, eHTTPConnection *content);
#endif /* __enigma_dyn_mount_h */

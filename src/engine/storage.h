/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_STORAGE_H
#define ENGINE_STORAGE_H

#include "kernel.h"

class IStorage : public IInterface
{
	MACRO_INTERFACE("storage", 0)
public:
	enum
	{
		TYPE_SAVE = 0,
		TYPE_ALL = -1,

		STORAGETYPE_BASIC = 0,
		STORAGETYPE_SERVER,
		STORAGETYPE_CLIENT,
	};

	virtual void ListDirectory(int Type, const char *pPath, FS_LISTDIR_CALLBACK pfnCallback, void *pUser) = 0;
	virtual IOHANDLE OpenFile(const char *pFilename, int Flags, int Type, char *pBuffer = 0, int BufferSize = 0) = 0;
	virtual bool FindFile(const char *pFilename, const char *pPath, int Type, char *pBuffer, int BufferSize) = 0;
	virtual bool RemoveFile(const char *pFilename, int Type) = 0;
	virtual bool RenameFile(const char* pOldFilename, const char* pNewFilename, int Type) = 0;
	virtual bool CreateFolder(const char *pFoldername, int Type) = 0;
	virtual void GetCompletePath(int Type, const char *pDir, char *pBuffer, unsigned BufferSize) = 0;
	virtual const char *MakeFullPath(char *pBuffer, unsigned BufferSize, int StorageType) const = 0;
	virtual const char *SandboxPath(char *pBuffer, unsigned BufferSize, const char *pPrepend = 0, bool ForcePrepend = false) const = 0;

};

extern IStorage *CreateStorage(const char *pApplicationName, int StorageType, int NumArgs, const char **ppArguments);


#endif

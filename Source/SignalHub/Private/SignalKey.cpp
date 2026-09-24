#include "SignalKey.h"

bool FSignalKey::operator==(const FSignalKey& InOther) const
{
	if (Storage == InOther.Storage)
	{
		return true;
	}
	if (!Storage.IsValid() || !InOther.Storage.IsValid() || Storage->GetTypeId() != InOther.Storage->GetTypeId())
	{
		return false;
	}
	return Storage->Equals(*InOther.Storage);
}

#include "MirandaContact.h"

#include <exception>
#include <sstream>

#include <m_contacts.h>
#include <m_core.h>

boost::optional<MirandaContact> MirandaContact::fromHandle(const HANDLE handle)
{
	return handle ? boost::make_optional(MirandaContact(handle)) : boost::optional<MirandaContact>();
}

boost::optional<MirandaContact> MirandaContact::getFirst()
{
	auto handle = db_find_first();
	return fromHandle(handle);
}

boost::optional<MirandaContact> MirandaContact::getNext(const MirandaContact &contact)
{
	auto prevHandle = contact.handle();
	auto handle = db_find_next(prevHandle);
	return fromHandle(handle);
}

MirandaContact::MirandaContact(const HANDLE handle)
	: _handle(handle)
{
}

HANDLE MirandaContact::handle() const
{
	return _handle;
}

std::wstring MirandaContact::uid() const
{
	auto contactInfo = CONTACTINFO();
	contactInfo.hContact = _handle;
	contactInfo.dwFlag = CNF_UNIQUEID | CNF_UNICODE;

	if (CallService(MS_CONTACT_GETCONTACTINFO, 0, reinterpret_cast<LPARAM>(&contactInfo)))
	{
		throw std::exception("Contact not found");
	}

	// There are unique identifiers of different types, so:
	auto stream = std::wostringstream();
	switch (contactInfo.type)
	{
	case CNFT_BYTE:
		stream << contactInfo.bVal;
		break;
	case CNFT_WORD:
		stream << contactInfo.wVal;
		break;
	case CNFT_DWORD:
		stream << contactInfo.dVal;
		break;
	case CNFT_ASCIIZ:
		stream << contactInfo.pszVal;
		break;
	default:
		throw std::exception("Unknown contact info value type");
	}

	return stream.str();
}
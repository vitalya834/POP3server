#include "headers/User.h"

User::User() : userName(""), password(""), locked(false) { }

User::User(const string &name, const string &pass) : userName(name), password(pass), locked(false) { }

User::~User()
{
}

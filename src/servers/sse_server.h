#pragma once


//==============================================================================
// `IStaticPointSseServer`
//==============================================================================


template <class DbDoc = Doc<>, class DbKw = Kw> requires IsValidDbParams<DbDoc, DbKw>
class IStaticPointSseServer {
    public:
        virtual void clear() = 0;
};

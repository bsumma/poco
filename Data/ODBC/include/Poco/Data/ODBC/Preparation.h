//
// Preparation.h
//
// $Id: //poco/1.3/Data/ODBC/include/Poco/Data/ODBC/Preparation.h#3 $
//
// Library: Data
// Package: DataCore
// Module:  Preparation
//
// Definition of the Preparation class.
//
// Copyright (c) 2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
// 
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//


#ifndef DataConnectors_ODBC_Preparation_INCLUDED
#define DataConnectors_ODBC_Preparation_INCLUDED


#include "Poco/Data/ODBC/ODBC.h"
#include "Poco/Data/ODBC/Handle.h"
#include "Poco/Data/ODBC/ODBCColumn.h"
#include "Poco/Data/ODBC/Utility.h"
#include "Poco/Data/AbstractPreparation.h"
#include "Poco/Data/BLOB.h"
#include "Poco/Any.h"
#include "Poco/SharedPtr.h"
#include <vector>
#ifdef POCO_OS_FAMILY_WINDOWS
#include <windows.h>
#endif
#include <sqlext.h>


namespace Poco {
namespace Data {
namespace ODBC {


class BLOB;


class ODBC_API Preparation : public AbstractPreparation
	/// Class used for database preparation where we first have to register all data types 
	/// with respective memory output locations before extracting data. 
	/// Extraction works in two-phases: first prepare is called once, then extract n-times.
	/// In ODBC, SQLBindCol/SQLFetch is the preferred method of data retrieval (SQLGetData is available, 
	/// however with numerous driver implementation dependent limitations). In order to fit this functionality 
	/// into Poco DataConnectors framework, every ODBC SQL statement instantiates its own Preparation object. 
	/// This is done once per statement execution (from StatementImpl::bindImpl()).
	///
	/// Preparation object is used to :
	///
	///		1) Prepare SQL statement.
	///		2) Provide and contain the memory locations where retrieved values are placed during recordset iteration.
	///		3) Keep count of returned number of columns with their respective datatypes and sizes.
	///
	/// Notes:
	///
	/// - Value datatypes in this interface prepare() calls serve only for the purpose of type distinction.
	/// - Preparation keeps its own std::vector<Any> buffer for fetched data to be later retrieved by Extractor.
	/// - prepare() methods should not be called when extraction mode is DE_MANUAL
	/// 
{
public:
	enum DataExtraction
	{
		DE_MANUAL,
		DE_BOUND
	};

	Preparation(const StatementHandle& rStmt, 
		const std::string& statement, 
		std::size_t maxFieldSize,
		DataExtraction dataExtraction = DE_BOUND);
		/// Creates the Preparation.

	~Preparation();
		/// Destroys the Preparation.

	void prepare(std::size_t pos, Poco::Int8);
		/// Prepares an Int8.

	void prepare(std::size_t pos, Poco::UInt8);
		/// Prepares an UInt8.

	void prepare(std::size_t pos, Poco::Int16);
		/// Prepares an Int16.

	void prepare(std::size_t pos, Poco::UInt16);
		/// Prepares an UInt16.

	void prepare(std::size_t pos, Poco::Int32);
		/// Prepares an Int32.

	void prepare(std::size_t pos, Poco::UInt32);
		/// Prepares an UInt32.

	void prepare(std::size_t pos, Poco::Int64);
		/// Prepares an Int64.

	void prepare(std::size_t pos, Poco::UInt64);
		/// Prepares an UInt64.

	void prepare(std::size_t pos, bool);
		/// Prepares a boolean.

	void prepare(std::size_t pos, float);
		/// Prepares a float.

	void prepare(std::size_t pos, double);
		/// Prepares a double.

	void prepare(std::size_t pos, char);
		/// Prepares a single character.

	void prepare(std::size_t pos, const std::string&);
		/// Prepares a string.

	void prepare(std::size_t pos, const Poco::Data::BLOB&);
		/// Prepares a BLOB.

	void prepare(std::size_t pos, const Poco::Any&);
		/// Prepares an Any.

	std::size_t columns() const;
		/// Returns the number of columns.

	Poco::Any& operator [] (std::size_t pos);
		/// Returns reference to column data.

	void setMaxFieldSize(std::size_t size);
		/// Sets maximum supported field size.

	std::size_t getMaxFieldSize() const;
		// Returns maximum supported field size.

	std::size_t maxDataSize(std::size_t pos) const;
		/// Returns max supported size for column at position pos.
		/// Returned length for variable length fields is the one 
		/// supported by this implementation, not the underlying DB.

	std::size_t actualDataSize(std::size_t pos) const;
		/// Returns the returned length. This is usually
		/// equal to the column size, except for variable length fields
		/// (BLOB and variable length strings).

	void setDataExtraction(DataExtraction ext);
		/// Set data extraction mode.

	DataExtraction getDataExtraction() const;
		/// Returns data extraction mode.

private:
	template <typename T>
	void preparePOD(std::size_t pos, SQLSMALLINT valueType)
	{
		poco_assert (DE_BOUND == _dataExtraction);
		poco_assert (pos >= 0 && pos < _pValues.size());

		std::size_t dataSize = sizeof(T);

		_pValues[pos] = new Poco::Any(T());
		_pLengths[pos] = new SQLLEN;
		*_pLengths[pos] = 0;

		T* pVal = AnyCast<T>(_pValues[pos]);

		poco_assert_dbg (pVal);

		if (Utility::isError(SQLBindCol(_rStmt, 
			(SQLUSMALLINT) pos + 1, 
			valueType, 
			(SQLPOINTER) pVal, 
			(SQLINTEGER) dataSize, 
			_pLengths[pos])))
		{
			throw StatementException(_rStmt, "SQLBindCol()");
		}
	}

	void prepareRaw(std::size_t pos, SQLSMALLINT valueType, std::size_t size);

	const StatementHandle& _rStmt;
	std::vector<Poco::Any*> _pValues;
	std::vector<SQLLEN*> _pLengths;
	std::size_t _maxFieldSize;
	DataExtraction _dataExtraction;
	std::vector<char*> _charPtrs;
};


//
// inlines
//
inline void Preparation::prepare(std::size_t pos, Poco::Int8)
{
	preparePOD<Poco::Int8>(pos, SQL_C_STINYINT);
}


inline void Preparation::prepare(std::size_t pos, Poco::UInt8)
{
	preparePOD<Poco::UInt8>(pos, SQL_C_UTINYINT);
}


inline void Preparation::prepare(std::size_t pos, Poco::Int16)
{
	preparePOD<Poco::Int16>(pos, SQL_C_SSHORT);
}


inline void Preparation::prepare(std::size_t pos, Poco::UInt16)
{
	preparePOD<Poco::UInt16>(pos, SQL_C_USHORT);
}


inline void Preparation::prepare(std::size_t pos, Poco::Int32)
{
	preparePOD<Poco::Int32>(pos, SQL_C_SLONG);
}


inline void Preparation::prepare(std::size_t pos, Poco::UInt32)
{
	preparePOD<Poco::UInt32>(pos, SQL_C_ULONG);
}


inline void Preparation::prepare(std::size_t pos, Poco::Int64)
{
	preparePOD<Poco::Int64>(pos, SQL_C_SBIGINT);
}


inline void Preparation::prepare(std::size_t pos, Poco::UInt64)
{
	preparePOD<Poco::UInt64>(pos, SQL_C_UBIGINT);
}


inline void Preparation::prepare(std::size_t pos, bool)
{
	preparePOD<bool>(pos, Utility::boolDataType);
}


inline void Preparation::prepare(std::size_t pos, float)
{
	preparePOD<float>(pos, SQL_C_FLOAT);
}


inline void Preparation::prepare(std::size_t pos, double)
{
	preparePOD<double>(pos, SQL_C_DOUBLE);
}


inline void Preparation::prepare(std::size_t pos, char)
{
	preparePOD<char>(pos, SQL_C_STINYINT);
}


inline void Preparation::prepare(std::size_t pos, const std::string&)
{
	prepareRaw(pos, SQL_C_CHAR, maxDataSize(pos));
}


inline void Preparation::prepare(std::size_t pos, const Poco::Data::BLOB&)
{
	prepareRaw(pos, SQL_C_BINARY, maxDataSize(pos));
}


inline std::size_t Preparation::columns() const
{
	return _pValues.size();
}


inline std::size_t Preparation::actualDataSize(std::size_t pos) const
{
	poco_assert (pos >= 0 && pos < _pValues.size());
	poco_assert (_pLengths[pos]);

	return *_pLengths[pos];
}


inline void Preparation::setMaxFieldSize(std::size_t size)
{
	_maxFieldSize = size;
}


inline std::size_t Preparation::getMaxFieldSize() const
{
	return _maxFieldSize;
}


inline void Preparation::setDataExtraction(Preparation::DataExtraction ext)
{
	_dataExtraction = ext;
}


inline Preparation::DataExtraction Preparation::getDataExtraction() const
{
	return _dataExtraction;
}


} } } // namespace Poco::Data::ODBC


#endif // DataConnectors_ODBC_Preparation_INCLUDED

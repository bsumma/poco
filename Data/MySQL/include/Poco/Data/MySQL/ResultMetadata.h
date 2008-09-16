//
// ResultMetadata.h
//
// $Id: //poco/1.3/Data/MySQL/include/Poco/Data/MySQL/ResultMetadata.h#1 $
//
// Library: Data
// Package: MySQL
// Module:  ResultMetadata
//
// Definition of the ResultMetadata class.
//
// Copyright (c) 2008, Applied Informatics Software Engineering GmbH.
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


#ifndef Data_MySQL_ResultMetadata_INCLUDED
#define Data_MySQL_ResultMetadata_INCLUDED


#include <mysql.h>
#include <vector>
#include "Poco/Data/MetaColumn.h"


namespace Poco {
namespace Data {
namespace MySQL {


class ResultMetadata
	/// MySQL result metadata
{
public:
	void reset();
		/// Resets the metadata.

	void init(MYSQL_STMT* stmt);
		/// Initializes the metadata.

	Poco::UInt32 columnsReturned() const;
		/// Returns the number of columns in resultset.

	const MetaColumn& metaColumn(Poco::UInt32 pos) const;
		/// Returns the reference to the specified metacolumn.

	MYSQL_BIND* row();
		/// Returns pointer to native row.

	std::size_t length(std::size_t pos) const;
		/// Returns the length.

	const char* rawData(std::size_t pos) const;
		/// Returns raw data.

	bool isNull(std::size_t pos) const;
		/// Returns true if value at pos is null.

private:
	std::vector<MetaColumn>    _columns;
	std::vector<MYSQL_BIND>    _row;
	std::vector<char>          _buffer;
	std::vector<unsigned long> _lengths;
    std::vector<my_bool>       _isNull;
};


} } } // namespace Poco::Data::MySQL


#endif //Data_MySQL_ResultMetadata_INCLUDED

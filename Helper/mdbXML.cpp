/*
www.sourceforge.net/projects/tinyxml
Original code (2.0 and earlier )copyright (c) 2000-2006 Lee Thomason (www.grinninglizard.com)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

#include <ctype.h>
#include <sstream>
#include <iostream>
#include "Helper/mdbXML.h"


bool MDBXMLBase::condenseWhiteSpace = true;

void MDBXMLBase::PutString( const string& str, string* outString )
{
	int i=0;

	while( i<(int)str.length() )
	{
		unsigned char c = (unsigned char) str[(long unsigned int)i];

		if (    c == '&' 
		     && i < ( (int)str.length() - 2 )
			 && str[(long unsigned int)(i+1)] == '#'
			 && str[(long unsigned int)(i+2)] == 'x' )
		{
			// Hexadecimal character reference.
			// Pass through unchanged.
			// &#xA9;	-- copyright symbol, for example.
			//
			// The -1 is a bug fix from Rob Laveaux. It keeps
			// an overflow from happening if there is no ';'.
			// There are actually 2 ways to exit this loop -
			// while fails (error case) and break (semicolon found).
			// However, there is no mechanism (currently) for
			// this function to return an error.
			while ( i<(int)str.length()-1 )
			{
				outString->append( str.c_str() + i, 1 );
				++i;
				if ( str[(long unsigned int)i] == ';' )
					break;
			}
		}
		else if ( c == '&' )
		{
			outString->append( entity[0].str, entity[0].strLength );
			++i;
		}
		else if ( c == '<' )
		{
			outString->append( entity[1].str, entity[1].strLength );
			++i;
		}
		else if ( c == '>' )
		{
			outString->append( entity[2].str, entity[2].strLength );
			++i;
		}
		else if ( c == '\"' )
		{
			outString->append( entity[3].str, entity[3].strLength );
			++i;
		}
		else if ( c == '\'' )
		{
			outString->append( entity[4].str, entity[4].strLength );
			++i;
		}
		else if ( c < 32 )
		{
			// Easy pass at non-alpha/numeric/symbol
			// Below 32 is symbolic.
			char buf[ 32 ];
			
			#if defined(MDBXML_SNPRINTF)		
				MDBXML_SNPRINTF( buf, sizeof(buf), "&#x%02X;", (unsigned) ( c & 0xff ) );
			#else
				sprintf( buf, "&#x%02X;", (unsigned) ( c & 0xff ) );
			#endif		

			//*ME:	warning C4267: convert 'size_t' to 'int'
			//*ME:	Int-Cast to make compiler happy ...
			outString->append( buf, strlen( buf ) );
			++i;
		}
		else
		{
			//char realc = (char) c;
			//outString->append( &realc, 1 );
			*outString += (char) c;	// somewhat more efficient function call.
			++i;
		}
	}
}


MDBXMLNode::MDBXMLNode( NodeType _type ) : MDBXMLBase()
{
	parent = 0;
	type = _type;
	firstChild = 0;
	lastChild = 0;
	prev = 0;
	next = 0;
}


MDBXMLNode::~MDBXMLNode()
{
	MDBXMLNode* node = firstChild;
	MDBXMLNode* temp = 0;

	while ( node )
	{
		temp = node;
		node = node->next;
		delete temp;
	}	
}


MDBXMLNode* MDBXMLNode::LastChild()	
{ 
    return lastChild; 
}

const MDBXMLNode* MDBXMLNode::LastChild() const	
{ 
    return lastChild; 
}		


void MDBXMLNode::CopyTo( MDBXMLNode* target ) const
{
	target->SetValue (value.c_str() );
	target->userData = userData; 
}


void MDBXMLNode::Clear()
{
	MDBXMLNode* node = firstChild;
	MDBXMLNode* temp = 0;

	while ( node )
	{
		temp = node;
		node = node->next;
		delete temp;
	}	

	firstChild = 0;
	lastChild = 0;
}

void MDBXMLNode::SetValue(const char * _value) 
{ 
    value.clear() ;
    value = _value;
}


MDBXMLNode* MDBXMLNode::LinkEndChild( MDBXMLNode* node )
{
	assert( node->parent == 0 || node->parent == this );
	assert( node->GetDocument() == 0 || node->GetDocument() == this->GetDocument() );

	if ( node->Type() == MDBXMLNode::DOCUMENT )
	{
		delete node;
		if ( GetDocument() ) GetDocument()->SetError( MDBXML_ERROR_DOCUMENT_TOP_ONLY, 0, 0, MDBXML_ENCODING_UNKNOWN );
		return 0;
	}

	node->parent = this;

	node->prev = lastChild;
	node->next = 0;

	if ( lastChild )
		lastChild->next = node;
	else
		firstChild = node;			// it was an empty list.

	lastChild = node;
	return node;
}


MDBXMLNode* MDBXMLNode::InsertEndChild( const MDBXMLNode& addThis )
{
	if ( addThis.Type() == MDBXMLNode::DOCUMENT )
	{
		if ( GetDocument() ) GetDocument()->SetError( MDBXML_ERROR_DOCUMENT_TOP_ONLY, 0, 0, MDBXML_ENCODING_UNKNOWN );
		return 0;
	}
	MDBXMLNode* node = addThis.Clone();
	if ( !node )
		return 0;

	return LinkEndChild( node );
}


MDBXMLNode* MDBXMLNode::InsertBeforeChild( MDBXMLNode* beforeThis, const MDBXMLNode& addThis )
{	
	if ( !beforeThis || beforeThis->parent != this ) {
		return 0;
	}
	if ( addThis.Type() == MDBXMLNode::DOCUMENT )
	{
		if ( GetDocument() ) GetDocument()->SetError( MDBXML_ERROR_DOCUMENT_TOP_ONLY, 0, 0, MDBXML_ENCODING_UNKNOWN );
		return 0;
	}

	MDBXMLNode* node = addThis.Clone();
	if ( !node )
		return 0;
	node->parent = this;

	node->next = beforeThis;
	node->prev = beforeThis->prev;
	if ( beforeThis->prev )
	{
		beforeThis->prev->next = node;
	}
	else
	{
		assert( firstChild == beforeThis );
		firstChild = node;
	}
	beforeThis->prev = node;
	return node;
}


MDBXMLNode* MDBXMLNode::InsertAfterChild( MDBXMLNode* afterThis, const MDBXMLNode& addThis )
{
	if ( !afterThis || afterThis->parent != this ) {
		return 0;
	}
	if ( addThis.Type() == MDBXMLNode::DOCUMENT )
	{
		if ( GetDocument() ) GetDocument()->SetError( MDBXML_ERROR_DOCUMENT_TOP_ONLY, 0, 0, MDBXML_ENCODING_UNKNOWN );
		return 0;
	}

	MDBXMLNode* node = addThis.Clone();
	if ( !node )
		return 0;
	node->parent = this;

	node->prev = afterThis;
	node->next = afterThis->next;
	if ( afterThis->next )
	{
		afterThis->next->prev = node;
	}
	else
	{
		assert( lastChild == afterThis );
		lastChild = node;
	}
	afterThis->next = node;
	return node;
}


MDBXMLNode* MDBXMLNode::ReplaceChild( MDBXMLNode* replaceThis, const MDBXMLNode& withThis )
{
	if ( replaceThis->parent != this )
		return 0;

	MDBXMLNode* node = withThis.Clone();
	if ( !node )
		return 0;

	node->next = replaceThis->next;
	node->prev = replaceThis->prev;

	if ( replaceThis->next )
		replaceThis->next->prev = node;
	else
		lastChild = node;

	if ( replaceThis->prev )
		replaceThis->prev->next = node;
	else
		firstChild = node;

	delete replaceThis;
	node->parent = this;
	return node;
}


bool MDBXMLNode::RemoveChild( MDBXMLNode* removeThis )
{
	if ( removeThis->parent != this )
	{	
		assert( 0 );
		return false;
	}

	if ( removeThis->next )
		removeThis->next->prev = removeThis->prev;
	else
		lastChild = removeThis->prev;

	if ( removeThis->prev )
		removeThis->prev->next = removeThis->next;
	else
		firstChild = removeThis->next;

	delete removeThis;
	return true;
}

const MDBXMLNode* MDBXMLNode::FirstChild( const char * _value ) const
{
	const MDBXMLNode* node;
	for ( node = firstChild; node; node = node->next )
	{
		if ( strcmp( node->Value(), _value ) == 0 )
			return node;
	}
	return 0;
}


const MDBXMLNode* MDBXMLNode::LastChild( const char * _value ) const
{
	const MDBXMLNode* node;
	for ( node = lastChild; node; node = node->prev )
	{
		if ( strcmp( node->Value(), _value ) == 0 )
			return node;
	}
	return 0;
}


const char * MDBXMLNode::Value()  const
{ 
    return value.c_str (); 
}

const MDBXMLNode* MDBXMLNode::IterateChildren( const MDBXMLNode* previous ) const
{
	if ( !previous )
	{
		return FirstChild();
	}
	else
	{
		assert( previous->parent == this );
		return previous->NextSibling();
	}
}


const MDBXMLNode* MDBXMLNode::IterateChildren( const char * val, const MDBXMLNode* previous ) const
{
	if ( !previous )
	{
		return FirstChild( val );
	}
	else
	{
		assert( previous->parent == this );
		return previous->NextSibling( val );
	}
}


const MDBXMLNode* MDBXMLNode::NextSibling() const
{ 
    return next; 
}

MDBXMLNode* MDBXMLNode::NextSibling() 
{ 
    return next; 
}

const MDBXMLNode* MDBXMLNode::NextSibling( const char * _value ) const 
{
	const MDBXMLNode* node;
	for ( node = next; node; node = node->next )
	{
		if ( strcmp( node->Value(), _value ) == 0 )
			return node;
	}
	return 0;
}


const MDBXMLNode* MDBXMLNode::PreviousSibling( const char * _value ) const
{
	const MDBXMLNode* node;
	for ( node = prev; node; node = node->prev )
	{
		if ( strcmp( node->Value(), _value ) == 0 )
			return node;
	}
	return 0;
}


const MDBXMLElement* MDBXMLElement::ToElement() const 
{ 
    return this; 
}

MDBXMLElement* MDBXMLElement::ToElement()
{ 
    return this; 
} 

void MDBXMLElement::RemoveAttribute( const char * name )
{

	string str( name );
	MDBXMLAttribute* node = attributeSet.Find( str );

	if ( node )
	{
		attributeSet.Remove( node );
		delete node;
	}
}

const MDBXMLElement* MDBXMLNode::FirstChildElement() const
{
	const MDBXMLNode* node;

	for (	node = FirstChild();
			node;
			node = node->NextSibling() )
	{
		if ( node->ToElement() )
			return node->ToElement();
	}
	return 0;
}


MDBXMLElement* MDBXMLNode::FirstChildElement() 
{
    return const_cast< MDBXMLElement* >( (const_cast< const MDBXMLNode* >(this))->FirstChildElement() );
}

MDBXMLElement* MDBXMLNode::FirstChildElement( const char * _value ) 
{
    return const_cast< MDBXMLElement* >( (const_cast< const MDBXMLNode* >(this))->FirstChildElement( _value ) );
}

const MDBXMLElement* MDBXMLNode::FirstChildElement( const char * _value ) const
{
	const MDBXMLNode* node;

	for (	node = FirstChild( _value );
			node;
			node = node->NextSibling( _value ) )
	{
		if ( node->ToElement() )
			return node->ToElement();
	}
	return 0;
}


const MDBXMLElement* MDBXMLNode::NextSiblingElement() const
{
	const MDBXMLNode* node;

	for (	node = NextSibling();
			node;
			node = node->NextSibling() )
	{
		if ( node->ToElement() )
			return node->ToElement();
	}
	return 0;
}


const MDBXMLElement* MDBXMLNode::NextSiblingElement( const char * _value ) const
{
	const MDBXMLNode* node;

	for (	node = NextSibling( _value );
			node;
			node = node->NextSibling( _value ) )
	{
		if ( node->ToElement() )
			return node->ToElement();
	}
	return 0;
}


const MDBXMLDocument* MDBXMLNode::GetDocument() const
{
	const MDBXMLNode* node;

	for( node = this; node; node = node->parent )
	{
		if ( node->ToDocument() )
			return node->ToDocument();
	}
	return 0;
}


MDBXMLElement::MDBXMLElement (const char * _value)
	: MDBXMLNode( MDBXMLNode::ELEMENT )
{
	firstChild = lastChild = 0;
	value = _value;
}



MDBXMLElement::MDBXMLElement( const std::string& _value ) 
	: MDBXMLNode( MDBXMLNode::ELEMENT )
{
	firstChild = lastChild = 0;
	value = _value;
}



MDBXMLElement::MDBXMLElement( const MDBXMLElement& copy)
	: MDBXMLNode( MDBXMLNode::ELEMENT )
{
	firstChild = lastChild = 0;
	copy.CopyTo( this );	
}


void MDBXMLElement::operator=( const MDBXMLElement& base )
{
	ClearThis();
	base.CopyTo( this );
}


MDBXMLElement::~MDBXMLElement()
{
	ClearThis();
}


void MDBXMLElement::ClearThis()
{
	Clear();
	while( attributeSet.First() )
	{
		MDBXMLAttribute* node = attributeSet.First();
		attributeSet.Remove( node );
		delete node;
	}
}


const char* MDBXMLElement::Attribute( const char* name ) const
{
	const MDBXMLAttribute* node = attributeSet.Find( name );
	if ( node )
		return node->Value();
	return 0;
}


const std::string* MDBXMLElement::Attribute( const std::string& name ) const
{
	const MDBXMLAttribute* node = attributeSet.Find( name );
	if ( node )
		return &node->ValueStr();
	return 0;
}



const char* MDBXMLElement::Attribute( const char* name, int* i ) const
{
	const char* s = Attribute( name );
	if ( i )
	{
		if ( s ) {
			*i = atoi( s );
		}
		else {
			*i = 0;
		}
	}
	return s;
}



const std::string* MDBXMLElement::Attribute( const std::string& name, int* i ) const
{
	const std::string* s = Attribute( name );
	if ( i )
	{
		if ( s ) {
			*i = atoi( s->c_str() );
		}
		else {
			*i = 0;
		}
	}
	return s;
}



const char* MDBXMLElement::Attribute( const char* name, double* d ) const
{
	const char* s = Attribute( name );
	if ( d )
	{
		if ( s ) {
			*d = atof( s );
		}
		else {
			*d = 0;
		}
	}
	return s;
}



const std::string* MDBXMLElement::Attribute( const std::string& name, double* d ) const
{
	const std::string* s = Attribute( name );
	if ( d )
	{
		if ( s ) {
			*d = atof( s->c_str() );
		}
		else {
			*d = 0;
		}
	}
	return s;
}



int MDBXMLElement::QueryIntAttribute( const char* name, int* ival ) const
{
	const MDBXMLAttribute* node = attributeSet.Find( name );
	if ( !node )
		return MDBXML_NO_ATTRIBUTE;
	return node->QueryIntValue( ival );
}



int MDBXMLElement::QueryIntAttribute( const std::string& name, int* ival ) const
{
	const MDBXMLAttribute* node = attributeSet.Find( name );
	if ( !node )
		return MDBXML_NO_ATTRIBUTE;
	return node->QueryIntValue( ival );
}



int MDBXMLElement::QueryDoubleAttribute( const char* name, double* dval ) const
{
	const MDBXMLAttribute* node = attributeSet.Find( name );
	if ( !node )
		return MDBXML_NO_ATTRIBUTE;
	return node->QueryDoubleValue( dval );
}



int MDBXMLElement::QueryDoubleAttribute( const std::string& name, double* dval ) const
{
	const MDBXMLAttribute* node = attributeSet.Find( name );
	if ( !node )
		return MDBXML_NO_ATTRIBUTE;
	return node->QueryDoubleValue( dval );
}



void MDBXMLElement::SetAttribute( const char * name, int val )
{	
	char buf[64];
	#if defined(MDBXML_SNPRINTF)		
		MDBXML_SNPRINTF( buf, sizeof(buf), "%d", val );
	#else
		sprintf( buf, "%d", val );
	#endif
	SetAttribute( name, buf );
}



void MDBXMLElement::SetAttribute( const std::string& name, int val )
{	
   std::ostringstream oss;
   oss << val;
   SetAttribute( name, oss.str() );
}



void MDBXMLElement::SetDoubleAttribute( const char * name, double val )
{	
	char buf[256];
	#if defined(MDBXML_SNPRINTF)		
		MDBXML_SNPRINTF( buf, sizeof(buf), "%f", val );
	#else
		sprintf( buf, "%f", val );
	#endif
	SetAttribute( name, buf );
}


void MDBXMLElement::SetAttribute( const char * cname, const char * cvalue )
{

	string _name( cname );
	string _value( cvalue );


	MDBXMLAttribute* node = attributeSet.Find( _name );
	if ( node )
	{
		node->SetValue( _value );
		return;
	}

	MDBXMLAttribute* attrib = new(std::nothrow) MDBXMLAttribute( cname, cvalue );
	if ( attrib )
	{
		attributeSet.Add( attrib );
	}
	else
	{
		MDBXMLDocument* document = GetDocument();
		if ( document ) document->SetError( MDBXML_ERROR_OUT_OF_MEMORY, 0, 0, MDBXML_ENCODING_UNKNOWN );
	}
}



void MDBXMLElement::SetAttribute( const std::string& name, const std::string& _value )
{
	MDBXMLAttribute* node = attributeSet.Find( name );
	if ( node )
	{
		node->SetValue( _value );
		return;
	}

	MDBXMLAttribute* attrib = new(std::nothrow) MDBXMLAttribute( name, _value );
	if ( attrib )
	{
		attributeSet.Add( attrib );
	}
	else
	{
		MDBXMLDocument* document = GetDocument();
		if ( document ) document->SetError( MDBXML_ERROR_OUT_OF_MEMORY, 0, 0, MDBXML_ENCODING_UNKNOWN );
	}
}



void MDBXMLElement::Print( FILE* cfile, int depth ) const
{
	int i;
	assert( cfile );
	for ( i=0; i<depth; i++ ) {
		fprintf( cfile, "    " );
	}

	fprintf( cfile, "<%s", value.c_str() );

	const MDBXMLAttribute* attrib;
	for ( attrib = attributeSet.First(); attrib; attrib = attrib->Next() )
	{
		fprintf( cfile, " " );
		attrib->Print( cfile, depth );
	}

	// There are 3 different formatting approaches:
	// 1) An element without children is printed as a <foo /> node
	// 2) An element with only a text child is printed as <foo> text </foo>
	// 3) An element with children is printed on multiple lines.
	MDBXMLNode* node;
	if ( !firstChild )
	{
		fprintf( cfile, " />" );
	}
	else if ( firstChild == lastChild && firstChild->ToText() )
	{
		fprintf( cfile, ">" );
		firstChild->Print( cfile, depth + 1 );
		fprintf( cfile, "</%s>", value.c_str() );
	}
	else
	{
		fprintf( cfile, ">" );

		for ( node = firstChild; node; node=node->NextSibling() )
		{
			if ( !node->ToText() )
			{
				fprintf( cfile, "\n" );
			}
			node->Print( cfile, depth+1 );
		}
		fprintf( cfile, "\n" );
		for( i=0; i<depth; ++i ) {
			fprintf( cfile, "    " );
		}
		fprintf( cfile, "</%s>", value.c_str() );
	}
}


void MDBXMLElement::CopyTo( MDBXMLElement* target ) const
{
	// superclass:
	MDBXMLNode::CopyTo( target );

	// Element class: 
	// Clone the attributes, then clone the children.
	const MDBXMLAttribute* attribute = 0;
	for(	attribute = attributeSet.First();
	attribute;
	attribute = attribute->Next() )
	{
		target->SetAttribute( attribute->Name(), attribute->Value() );
	}

	MDBXMLNode* node = 0;
	for ( node = firstChild; node; node = node->NextSibling() )
	{
		target->LinkEndChild( node->Clone() );
	}
}

bool MDBXMLElement::Accept( MDBXMLVisitor* visitor ) const
{
	if ( visitor->VisitEnter( *this, attributeSet.First() ) ) 
	{
		for ( const MDBXMLNode* node=FirstChild(); node; node=node->NextSibling() )
		{
			if ( !node->Accept( visitor ) )
				break;
		}
	}
	return visitor->VisitExit( *this );
}


MDBXMLNode* MDBXMLElement::Clone() const
{
	MDBXMLElement* clone = new(std::nothrow) MDBXMLElement( Value() );
	if ( !clone )
		return 0;

	CopyTo( clone );
	return clone;
}


const char* MDBXMLElement::GetText() const
{
	const MDBXMLNode* child = this->FirstChild();
	if ( child ) {
		const MDBXMLText* childText = child->ToText();
		if ( childText ) {
			return childText->Value();
		}
	}
	return 0;
}


MDBXMLDocument::MDBXMLDocument() : MDBXMLNode( MDBXMLNode::DOCUMENT )
{
	tabsize = 4;
	useMicrosoftBOM = false;
	ClearError();
}

MDBXMLDocument::MDBXMLDocument( const char * documentName ) : MDBXMLNode( MDBXMLNode::DOCUMENT )
{
	tabsize = 4;
	useMicrosoftBOM = false;
	value = documentName;
	ClearError();
}



MDBXMLDocument::MDBXMLDocument( const std::string& documentName ) : MDBXMLNode( MDBXMLNode::DOCUMENT )
{
	tabsize = 4;
	useMicrosoftBOM = false;
    value = documentName;
	ClearError();
}



MDBXMLDocument::MDBXMLDocument( const MDBXMLDocument& copy ) : MDBXMLNode( MDBXMLNode::DOCUMENT )
{
	copy.CopyTo( this );
}


void MDBXMLDocument::operator=( const MDBXMLDocument& copy )
{
	Clear();
	copy.CopyTo( this );
}


bool MDBXMLDocument::LoadFile( MDBXMLEncoding encoding )
{
	// See STL_STRING_BUG below.
	//StringToBuffer buf( value );

	return LoadFile( Value(), encoding );
}


bool MDBXMLDocument::SaveFile() const
{
	// See STL_STRING_BUG below.
//	StringToBuffer buf( value );
//
//	if ( buf.buffer && SaveFile( buf.buffer ) )
//		return true;
//
//	return false;
	return SaveFile( Value() );
}

bool MDBXMLDocument::LoadFile( const char* _filename, MDBXMLEncoding encoding )
{
	// There was a really terrifying little bug here. The code:
	//		value = filename
	// in the STL case, cause the assignment method of the std::string to
	// be called. What is strange, is that the std::string had the same
	// address as it's c_str() method, and so bad things happen. Looks
	// like a bug in the Microsoft STL implementation.
	// Add an extra string to avoid the crash.
	string filename( _filename );
	value = filename;

	// reading in binary mode so that tinyxml can normalize the EOL
	FILE* file = fopen( value.c_str (), "rb" );	

	if ( file )
	{
		bool result = LoadFile( file, encoding );
		fclose( file );
		return result;
	}
	else
	{
		SetError( MDBXML_ERROR_OPENING_FILE, 0, 0, MDBXML_ENCODING_UNKNOWN );
		return false;
	}
}

bool MDBXMLDocument::LoadFile( FILE* file, MDBXMLEncoding encoding )
{
	if ( !file ) 
	{
		SetError( MDBXML_ERROR_OPENING_FILE, 0, 0, MDBXML_ENCODING_UNKNOWN );
		return false;
	}

	// Delete the existing data:
	Clear();
	location.Clear();

	// Get the file size, so we can pre-allocate the string. HUGE speed impact.
	long length = 0;
	fseek( file, 0, SEEK_END );
	length = ftell( file );
	fseek( file, 0, SEEK_SET );

	// Strange case, but good to handle up front.
	if ( length == 0 )
	{
		SetError( MDBXML_ERROR_DOCUMENT_EMPTY, 0, 0, MDBXML_ENCODING_UNKNOWN );
		return false;
	}

	// If we have a file, assume it is all one big XML file, and read it in.
	// The document parser may decide the document ends sooner than the entire file, however.
	string data;
	data.reserve( (long unsigned int)length );

	// Subtle bug here. TinyXml did use fgets. But from the XML spec:
	// 2.11 End-of-Line Handling
	// <snip>
	// <quote>
	// ...the XML processor MUST behave as if it normalized all line breaks in external 
	// parsed entities (including the document entity) on input, before parsing, by translating 
	// both the two-character sequence #xD #xA and any #xD that is not followed by #xA to 
	// a single #xA character.
	// </quote>
	//
	// It is not clear fgets does that, and certainly isn't clear it works cross platform. 
	// Generally, you expect fgets to translate from the convention of the OS to the c/unix
	// convention, and not work generally.

	/*
	while( fgets( buf, sizeof(buf), file ) )
	{
		data += buf;
	}
	*/

	char* buf = new char[ length+1 ];
	buf[0] = 0;

	if ( fread( buf, (size_t)length, 1, file ) != 1 ) {
		delete [] buf;
		SetError( MDBXML_ERROR_OPENING_FILE, 0, 0, MDBXML_ENCODING_UNKNOWN );
		return false;
	}

	const char* lastPos = buf;
	const char* p = buf;

	buf[length] = 0;
	while( *p ) {
		assert( p < (buf+length) );
		if ( *p == 0xa ) {
			// Newline character. No special rules for this. Append all the characters
			// since the last string, and include the newline.
			data.append( lastPos, (long unsigned int)(p-lastPos+1) );	// append, include the newline
			++p;									// move past the newline
			lastPos = p;							// and point to the new buffer (may be 0)
			assert( p <= (buf+length) );
		}
		else if ( *p == 0xd ) {
			// Carriage return. Append what we have so far, then
			// handle moving forward in the buffer.
			if ( (p-lastPos) > 0 ) {
				data.append( lastPos, (long unsigned int)(p-lastPos) );	// do not add the CR
			}
			data += (char)0xa;						// a proper newline

			if ( *(p+1) == 0xa ) {
				// Carriage return - new line sequence
				p += 2;
				lastPos = p;
				assert( p <= (buf+length) );
			}
			else {
				// it was followed by something else...that is presumably characters again.
				++p;
				lastPos = p;
				assert( p <= (buf+length) );
			}
		}
		else {
			++p;
		}
	}
	// Handle any left over characters.
	if ( p-lastPos ) {
		data.append( lastPos,(long unsigned int)(p-lastPos) );
	}		
	delete [] buf;
	buf = 0;

	Parse( data.c_str(), 0, encoding );

	if (  Error() )
        return false;
    else
		return true;
}


bool MDBXMLDocument::SaveFile( const char * filename ) const
{
	// The old c stuff lives on...
	FILE* fp = fopen( filename, "w" );
	if ( fp )
	{
		bool result = SaveFile( fp );
		fclose( fp );
		return result;
	}
	return false;
}


bool MDBXMLDocument::SaveFile( FILE* fp ) const
{
	if ( useMicrosoftBOM ) 
	{
		const unsigned char MDBXML_UTF_LEAD_0 = 0xefU;
		const unsigned char MDBXML_UTF_LEAD_1 = 0xbbU;
		const unsigned char MDBXML_UTF_LEAD_2 = 0xbfU;

		fputc( MDBXML_UTF_LEAD_0, fp );
		fputc( MDBXML_UTF_LEAD_1, fp );
		fputc( MDBXML_UTF_LEAD_2, fp );
	}
	Print( fp, 0 );
	return (ferror(fp) == 0);
}


void MDBXMLDocument::CopyTo( MDBXMLDocument* target ) const
{
	MDBXMLNode::CopyTo( target );

	target->error = error;
	target->errorDesc = errorDesc.c_str ();

	MDBXMLNode* node = 0;
	for ( node = firstChild; node; node = node->NextSibling() )
	{
		target->LinkEndChild( node->Clone() );
	}	
}


MDBXMLNode* MDBXMLDocument::Clone() const
{
	MDBXMLDocument* clone = new(std::nothrow) MDBXMLDocument();
	if ( !clone )
		return 0;

	CopyTo( clone );
	return clone;
}


void MDBXMLDocument::Print( FILE* cfile, int depth ) const
{
	assert( cfile );
	for ( const MDBXMLNode* node=FirstChild(); node; node=node->NextSibling() )
	{
		node->Print( cfile, depth );
		fprintf( cfile, "\n" );
	}
}


bool MDBXMLDocument::Accept( MDBXMLVisitor* visitor ) const
{
	if ( visitor->VisitEnter( *this ) )
	{
		for ( const MDBXMLNode* node=FirstChild(); node; node=node->NextSibling() )
		{
			if ( !node->Accept( visitor ) )
				break;
		}
	}
	return visitor->VisitExit( *this );
}


const MDBXMLAttribute* MDBXMLAttribute::Next() const
{
	// We are using knowledge of the sentinel. The sentinel
	// have a value or name.
	if ( next->value.empty() && next->name.empty() )
		return 0;
	return next;
}

/*
MDBXMLAttribute* MDBXMLAttribute::Next()
{
	// We are using knowledge of the sentinel. The sentinel
	// have a value or name.
	if ( next->value.empty() && next->name.empty() )
		return 0;
	return next;
}
*/

const MDBXMLAttribute* MDBXMLAttribute::Previous() const
{
	// We are using knowledge of the sentinel. The sentinel
	// have a value or name.
	if ( prev->value.empty() && prev->name.empty() )
		return 0;
	return prev;
}

/*
MDBXMLAttribute* MDBXMLAttribute::Previous()
{
	// We are using knowledge of the sentinel. The sentinel
	// have a value or name.
	if ( prev->value.empty() && prev->name.empty() )
		return 0;
	return prev;
}
*/

void MDBXMLAttribute::Print( FILE* cfile, int /*depth*/, string* str ) const
{
	string n, v;

	PutString( name, &n );
	PutString( value, &v );

	if (value.find ('\"') == string::npos) {
		if ( cfile ) {
		fprintf (cfile, "%s=\"%s\"", n.c_str(), v.c_str() );
		}
		if ( str ) {
			(*str) += n; (*str) += "=\""; (*str) += v; (*str) += "\"";
		}
	}
	else {
		if ( cfile ) {
		fprintf (cfile, "%s='%s'", n.c_str(), v.c_str() );
		}
		if ( str ) {
			(*str) += n; (*str) += "='"; (*str) += v; (*str) += "'";
		}
	}
}


int MDBXMLAttribute::QueryIntValue( int* ival ) const
{
	if ( sscanf( value.c_str(), "%d", ival ) == 1 )
		return MDBXML_SUCCESS;
	return MDBXML_WRONG_TYPE;
}

int MDBXMLAttribute::QueryDoubleValue( double* dval ) const
{
	if ( sscanf( value.c_str(), "%lf", dval ) == 1 )
		return MDBXML_SUCCESS;
	return MDBXML_WRONG_TYPE;
}

void MDBXMLAttribute::SetIntValue( int _value )
{
	char buf [64];
	#if defined(MDBXML_SNPRINTF)		
		MDBXML_SNPRINTF(buf, sizeof(buf), "%d", _value);
	#else
		sprintf (buf, "%d", _value);
	#endif
	SetValue (buf);
}

void MDBXMLAttribute::SetDoubleValue( double _value )
{
	char buf [256];
	#if defined(MDBXML_SNPRINTF)		
		MDBXML_SNPRINTF( buf, sizeof(buf), "%lf", _value);
	#else
		sprintf (buf, "%lf", _value);
	#endif
	SetValue (buf);
}

int MDBXMLAttribute::IntValue() const
{
	return atoi (value.c_str ());
}

double  MDBXMLAttribute::DoubleValue() const
{
	return atof (value.c_str ());
}


MDBXMLComment::MDBXMLComment( const MDBXMLComment& copy ) : MDBXMLNode( MDBXMLNode::COMMENT )
{
	copy.CopyTo( this );
}


void MDBXMLComment::operator=( const MDBXMLComment& base )
{
	Clear();
	base.CopyTo( this );
}


void MDBXMLComment::Print( FILE* cfile, int depth ) const
{
	assert( cfile );
	for ( int i=0; i<depth; i++ )
	{
		fprintf( cfile,  "    " );
	}
	fprintf( cfile, "<!--%s-->", value.c_str() );
}


void MDBXMLComment::CopyTo( MDBXMLComment* target ) const
{
	MDBXMLNode::CopyTo( target );
}


bool MDBXMLComment::Accept( MDBXMLVisitor* visitor ) const
{
	return visitor->Visit( *this );
}


MDBXMLNode* MDBXMLComment::Clone() const
{
	MDBXMLComment* clone = new(std::nothrow) MDBXMLComment();

	if ( !clone )
		return 0;

	CopyTo( clone );
	return clone;
}


MDBXMLText::MDBXMLText (const char * sValue ) : MDBXMLNode (MDBXMLNode::TEXT)
{
    //SetValue( initValue );
    value = sValue ;
    cdata = false;
}


void MDBXMLText::Print( FILE* cfile, int depth ) const
{
	assert( cfile );
	if ( cdata )
	{
		int i;
		fprintf( cfile, "\n" );
		for ( i=0; i<depth; i++ ) {
			fprintf( cfile, "    " );
		}
		fprintf( cfile, "<![CDATA[%s]]>\n", value.c_str() );	// unformatted output
	}
	else
	{
		string buffer;
		PutString( value, &buffer );
		fprintf( cfile, "%s", buffer.c_str() );
	}
}


void MDBXMLText::CopyTo( MDBXMLText* target ) const
{
	MDBXMLNode::CopyTo( target );
	target->cdata = cdata;
}


bool MDBXMLText::Accept( MDBXMLVisitor* visitor ) const
{
	return visitor->Visit( *this );
}


MDBXMLNode* MDBXMLText::Clone() const
{	
	MDBXMLText* clone = 0;
	clone = new(std::nothrow) MDBXMLText( "" );

	if ( !clone )
		return 0;

	CopyTo( clone );
	return clone;
}


MDBXMLDeclaration::MDBXMLDeclaration( const char * _version,
									const char * _encoding,
									const char * _standalone )
	: MDBXMLNode( MDBXMLNode::DECLARATION )
{
	version = _version;
	encoding = _encoding;
	standalone = _standalone;
}



MDBXMLDeclaration::MDBXMLDeclaration(	const std::string& _version,
									const std::string& _encoding,
									const std::string& _standalone )
	: MDBXMLNode( MDBXMLNode::DECLARATION )
{
	version = _version;
	encoding = _encoding;
	standalone = _standalone;
}



MDBXMLDeclaration::MDBXMLDeclaration( const MDBXMLDeclaration& copy )
	: MDBXMLNode( MDBXMLNode::DECLARATION )
{
	copy.CopyTo( this );	
}


void MDBXMLDeclaration::operator=( const MDBXMLDeclaration& copy )
{
	Clear();
	copy.CopyTo( this );
}


void MDBXMLDeclaration::Print( FILE* cfile, int /*depth*/, string* str ) const
{
	if ( cfile ) fprintf( cfile, "<?xml " );
	if ( str )	 (*str) += "<?xml ";

	if ( !version.empty() ) {
		if ( cfile ) fprintf (cfile, "version=\"%s\" ", version.c_str ());
		if ( str ) { (*str) += "version=\""; (*str) += version; (*str) += "\" "; }
	}
	if ( !encoding.empty() ) {
		if ( cfile ) fprintf (cfile, "encoding=\"%s\" ", encoding.c_str ());
		if ( str ) { (*str) += "encoding=\""; (*str) += encoding; (*str) += "\" "; }
	}
	if ( !standalone.empty() ) {
		if ( cfile ) fprintf (cfile, "standalone=\"%s\" ", standalone.c_str ());
		if ( str ) { (*str) += "standalone=\""; (*str) += standalone; (*str) += "\" "; }
	}
	if ( cfile ) fprintf( cfile, "?>" );
	if ( str )	 (*str) += "?>";
}


void MDBXMLDeclaration::CopyTo( MDBXMLDeclaration* target ) const
{
	MDBXMLNode::CopyTo( target );

	target->version = version;
	target->encoding = encoding;
	target->standalone = standalone;
}


bool MDBXMLDeclaration::Accept( MDBXMLVisitor* visitor ) const
{
	return visitor->Visit( *this );
}


MDBXMLNode* MDBXMLDeclaration::Clone() const
{	
	MDBXMLDeclaration* clone = new(std::nothrow) MDBXMLDeclaration();

	if ( !clone )
		return 0;

	CopyTo( clone );
	return clone;
}


void MDBXMLUnknown::Print( FILE* cfile, int depth ) const
{
	for ( int i=0; i<depth; i++ )
		fprintf( cfile, "    " );
	fprintf( cfile, "<%s>", value.c_str() );
}


void MDBXMLUnknown::CopyTo( MDBXMLUnknown* target ) const
{
	MDBXMLNode::CopyTo( target );
}


bool MDBXMLUnknown::Accept( MDBXMLVisitor* visitor ) const
{
	return visitor->Visit( *this );
}


MDBXMLNode* MDBXMLUnknown::Clone() const
{
	MDBXMLUnknown* clone = new(std::nothrow) MDBXMLUnknown();

	if ( !clone )
		return 0;

	CopyTo( clone );
	return clone;
}


MDBXMLAttributeSet::MDBXMLAttributeSet()
{
	sentinel.next = &sentinel;
	sentinel.prev = &sentinel;
}


MDBXMLAttributeSet::~MDBXMLAttributeSet()
{
	assert( sentinel.next == &sentinel );
	assert( sentinel.prev == &sentinel );
}


void MDBXMLAttributeSet::Add( MDBXMLAttribute* addMe )
{

	assert( !Find( string( addMe->Name() ) ) );	// Shouldn't be multiply adding to the set.


	addMe->next = &sentinel;
	addMe->prev = sentinel.prev;

	sentinel.prev->next = addMe;
	sentinel.prev      = addMe;
}

void MDBXMLAttributeSet::Remove( MDBXMLAttribute* removeMe )
{
	MDBXMLAttribute* node;

	for( node = sentinel.next; node != &sentinel; node = node->next )
	{
		if ( node == removeMe )
		{
			node->prev->next = node->next;
			node->next->prev = node->prev;
			node->next = 0;
			node->prev = 0;
			return;
		}
	}
	assert( 0 );		// we tried to remove a non-linked attribute.
}



const MDBXMLAttribute* MDBXMLAttributeSet::Find( const std::string& name ) const
{
	for( const MDBXMLAttribute* node = sentinel.next; node != &sentinel; node = node->next )
	{
		if ( node->name == name )
			return node;
	}
	return 0;
}

/*
MDBXMLAttribute*	MDBXMLAttributeSet::Find( const std::string& name )
{
	for( MDBXMLAttribute* node = sentinel.next; node != &sentinel; node = node->next )
	{
		if ( node->name == name )
			return node;
	}
	return 0;
}
*/



const MDBXMLAttribute* MDBXMLAttributeSet::Find( const char* name ) const
{
	for( const MDBXMLAttribute* node = sentinel.next; node != &sentinel; node = node->next )
	{
		if ( strcmp( node->name.c_str(), name ) == 0 )
			return node;
	}
	return 0;
}

/*
MDBXMLAttribute*	MDBXMLAttributeSet::Find( const char* name )
{
	for( MDBXMLAttribute* node = sentinel.next; node != &sentinel; node = node->next )
	{
		if ( strcmp( node->name.c_str(), name ) == 0 )
			return node;
	}
	return 0;
}
*/


std::istream& operator>> (std::istream & in, MDBXMLNode & base)
{
	string tag;
	tag.reserve( 8 * 1000 );
	base.StreamIn( &in, &tag );

	base.Parse( tag.c_str(), 0, MDBXML_DEFAULT_ENCODING );
	return in;
}




std::ostream& operator<< (std::ostream & out, const MDBXMLNode & base)
{
	MDBXMLPrinter printer;
	printer.SetStreamPrinting();
	base.Accept( &printer );
	out << printer.Str();

	return out;
}


std::string& operator<< (std::string& out, const MDBXMLNode& base )
{
	MDBXMLPrinter printer;
	printer.SetStreamPrinting();
	base.Accept( &printer );
	out.append( printer.Str() );

	return out;
}


MDBXMLHandle MDBXMLHandle::FirstChild() const
{
	if ( node )
	{
		MDBXMLNode* child = node->FirstChild();
		if ( child )
			return MDBXMLHandle( child );
	}
	return MDBXMLHandle( 0 );
}


MDBXMLHandle MDBXMLHandle::FirstChild( const char * value ) const
{
	if ( node )
	{
		MDBXMLNode* child = node->FirstChild( value );
		if ( child )
			return MDBXMLHandle( child );
	}
	return MDBXMLHandle( 0 );
}


MDBXMLHandle MDBXMLHandle::FirstChildElement() const
{
	if ( node )
	{
		MDBXMLElement* child = node->FirstChildElement();
		if ( child )
			return MDBXMLHandle( child );
	}
	return MDBXMLHandle( 0 );
}


MDBXMLHandle MDBXMLHandle::FirstChildElement( const char * value ) const
{
	if ( node )
	{
		MDBXMLElement* child = node->FirstChildElement( value );
		if ( child )
			return MDBXMLHandle( child );
	}
	return MDBXMLHandle( 0 );
}


MDBXMLHandle MDBXMLHandle::Child( int count ) const
{
	if ( node )
	{
		int i;
		MDBXMLNode* child = node->FirstChild();
		for (	i=0;
				child && i<count;
				child = child->NextSibling(), ++i )
		{
			// nothing
		}
		if ( child )
			return MDBXMLHandle( child );
	}
	return MDBXMLHandle( 0 );
}


MDBXMLHandle MDBXMLHandle::Child( const char* value, int count ) const
{
	if ( node )
	{
		int i;
		MDBXMLNode* child = node->FirstChild( value );
		for (	i=0;
				child && i<count;
				child = child->NextSibling( value ), ++i )
		{
			// nothing
		}
		if ( child )
			return MDBXMLHandle( child );
	}
	return MDBXMLHandle( 0 );
}


MDBXMLHandle MDBXMLHandle::ChildElement( int count ) const
{
	if ( node )
	{
		int i;
		MDBXMLElement* child = node->FirstChildElement();
		for (	i=0;
				child && i<count;
				child = child->NextSiblingElement(), ++i )
		{
			// nothing
		}
		if ( child )
			return MDBXMLHandle( child );
	}
	return MDBXMLHandle( 0 );
}


MDBXMLHandle MDBXMLHandle::ChildElement( const char* value, int count ) const
{
	if ( node )
	{
		int i;
		MDBXMLElement* child = node->FirstChildElement( value );
		for (	i=0;
				child && i<count;
				child = child->NextSiblingElement( value ), ++i )
		{
			// nothing
		}
		if ( child )
			return MDBXMLHandle( child );
	}
	return MDBXMLHandle( 0 );
}


bool MDBXMLPrinter::VisitEnter( const MDBXMLDocument& )
{
	return true;
}

bool MDBXMLPrinter::VisitExit( const MDBXMLDocument& )
{
	return true;
}

bool MDBXMLPrinter::VisitEnter( const MDBXMLElement& element, const MDBXMLAttribute* firstAttribute )
{
	DoIndent();
	buffer += "<";
	buffer += element.Value();

	for( const MDBXMLAttribute* attrib = firstAttribute; attrib; attrib = attrib->Next() )
	{
		buffer += " ";
		attrib->Print( 0, 0, &buffer );
	}

	if ( !element.FirstChild() ) 
	{
		buffer += " />";
		DoLineBreak();
	}
	else 
	{
		buffer += ">";
		if (    element.FirstChild()->ToText()
			  && element.LastChild() == element.FirstChild()
			  && element.FirstChild()->ToText()->CDATA() == false )
		{
			simpleTextPrint = true;
			// no DoLineBreak()!
		}
		else
		{
			DoLineBreak();
		}
	}
	++depth;	
	return true;
}


bool MDBXMLPrinter::VisitExit( const MDBXMLElement& element )
{
	--depth;
	if ( !element.FirstChild() ) 
	{
		// nothing.
	}
	else 
	{
		if ( simpleTextPrint )
		{
			simpleTextPrint = false;
		}
		else
		{
			DoIndent();
		}
		buffer += "</";
		buffer += element.Value();
		buffer += ">";
		DoLineBreak();
	}
	return true;
}


bool MDBXMLPrinter::Visit( const MDBXMLText& text )
{
	if ( text.CDATA() )
	{
		DoIndent();
		buffer += "<![CDATA[";
		buffer += text.Value();
		buffer += "]]>";
		DoLineBreak();
	}
	else if ( simpleTextPrint )
	{
		buffer += text.Value();
	}
	else
	{
		DoIndent();
		buffer += text.Value();
		DoLineBreak();
	}
	return true;
}


bool MDBXMLPrinter::Visit( const MDBXMLDeclaration& declaration )
{
	DoIndent();
	declaration.Print( 0, 0, &buffer );
	DoLineBreak();
	return true;
}


bool MDBXMLPrinter::Visit( const MDBXMLComment& comment )
{
	DoIndent();
	buffer += "<!--";
	buffer += comment.Value();
	buffer += "-->";
	DoLineBreak();
	return true;
}


bool MDBXMLPrinter::Visit( const MDBXMLUnknown& unknown )
{
	DoIndent();
	buffer += "<";
	buffer += unknown.Value();
	buffer += ">";
	DoLineBreak();
	return true;
}


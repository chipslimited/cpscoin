/**
 *  @file
 *  @copyright defined in cps/LICENSE.txt
 */
#pragma once
namespace cpsio { namespace types {
   template<typename T>
   fc::variant toVariant( const vector<T>& a ) {
      vector<fc::variant> result;
      result.reserve( a.size() );
      for( const auto& v : a )
         result.emplace_back( cpsio::toVariant( v ) );
      return fc::variant( std::move(result) );
   }

   template<typename Stream, typename T>
   void toBinary( Stream& s, const vector<T>& a ) {
      uint32_t len = a.size();
      s.write( (const char*)&len, sizeof(len) );
      for( const auto& v : a )
         cpsio::toBinary( s, v );
   }

   template<typename Stream, typename T>
   void fromBinary( Stream& s, vector<T>& a ) {
      uint32_t len = 0;
      s.read( (char*)&len, sizeof(len) );
      a.resize(len);
      for( auto& v : a )
         cpsio::fromBinary( s, v );
   }

   template<typename T>
   void fromVariant( vector<T>& data, const fc::variant& v ) {
      const auto& a = v.get_array();
      data.resize( a.size() );
      for( uint32_t i = 0; i < a.size(); ++i )
         cpsio::fromVariant( data[i], a[i] );
   }

   template<typename T>
   T Bytes::as()const {
     fc::datastream<const char*> ds( value.data(), value.size() );
     T result;
     cpsio::fromBinary( ds, result );
     return result;
   }
}} // namespace cpsio::types

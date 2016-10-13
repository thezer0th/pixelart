// Copyright (c) 2014-2015 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/ColinH/PEGTL/

#ifndef TAOCPP_JSON_EMBEDDED_PEGTL_ANALYSIS_ANALYZE_CYCLES_HH
#define TAOCPP_JSON_EMBEDDED_PEGTL_ANALYSIS_ANALYZE_CYCLES_HH

#include <cassert>

#include <map>
#include <set>

#include <utility>
#include <iostream>

#include "grammar_info.hh"
#include "insert_guard.hh"

namespace tao_json_pegtl
{
   namespace analysis
   {
      class analyze_cycles_impl
      {
      protected:
         explicit
         analyze_cycles_impl( const bool verbose )
               : m_verbose( verbose ),
                 m_problems( 0 )
         { }

         const bool m_verbose;
         unsigned m_problems;
         grammar_info m_info;
         std::set< std::string > m_stack;
         std::map< std::string, bool > m_cache;
         std::map< std::string, bool > m_results;

         const std::map< std::string, rule_info >::const_iterator find( const std::string & name ) const
         {
            const auto iter = m_info.map.find( name );
            assert( iter != m_info.map.end() );
            return iter;
         }

         bool work( const std::map< std::string, rule_info >::const_iterator & start, const bool accum )
         {
            const auto j = m_cache.find( start->first );

            if ( j != m_cache.end() ) {
               return j->second;
            }
            if ( const auto g = make_insert_guard( m_stack, start->first ) ) {
               switch ( start->second.type ) {
                  case rule_type::ANY: {
                     bool a = false;
                     for ( const auto & r : start->second.rules ) {
                        a |= work( find( r ), accum || a );
                     }
                     return m_cache[ start->first ] = true;
                  }
                  case rule_type::OPT: {
                     bool a = false;
                     for ( const auto & r : start->second.rules ) {
                        a |= work( find( r ), accum || a );
                     }
                     return m_cache[ start->first ] = false;
                  }
                  case rule_type::SEQ: {
                     bool a = false;
                     for ( const auto & r : start->second.rules ) {
                        a |= work( find( r ), accum || a );
                     }
                     return m_cache[ start->first ] = a;
                  }
                  case rule_type::SOR: {
                     bool a = true;
                     for ( const auto & r : start->second.rules ) {
                        a &= work( find( r ), accum );
                     }
                     return m_cache[ start->first ] = a;
                  }
               }
               assert( false );  // LCOV_EXCL_LINE
            }
            if ( ! accum ) {
               ++m_problems;
               if ( m_verbose ) {
                  std::cout << "problem: cycle without progress detected at rule class " << start->first << std::endl;
               }
            }
            return m_cache[ start->first ] = accum;
         }
      };

      template< typename Grammar >
      class analyze_cycles
            : private analyze_cycles_impl
      {
      public:
         explicit
         analyze_cycles( const bool verbose )
               : analyze_cycles_impl( verbose )
         {
            Grammar::analyze_t::template insert< Grammar >( m_info );
         }

         std::size_t problems()
         {
            for ( auto i = m_info.map.begin(); i != m_info.map.end(); ++i ) {
               m_results[ i->first ] = work( i, false );
               m_cache.clear();
            }
            return m_problems;
         }

         template< typename Rule >
         bool consumes() const
         {
            const auto i = m_results.find( internal::demangle< Rule >() );
            assert( i != m_results.end() );
            return i->second;
         }
      };

   } // analysis

} // tao_json_pegtl

#endif

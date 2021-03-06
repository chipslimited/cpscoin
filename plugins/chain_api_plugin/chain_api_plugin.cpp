/**
 *  @file
 *  @copyright defined in cps/LICENSE.txt
 */
#include <cps/chain_api_plugin/chain_api_plugin.hpp>
#include <cps/chain/exceptions.hpp>

#include <fc/io/json.hpp>

namespace cpsio {

using namespace cpsio;

class chain_api_plugin_impl {
public:
   chain_api_plugin_impl(chain_controller& db)
      : db(db) {}

   chain_controller& db;
};


chain_api_plugin::chain_api_plugin(){}
chain_api_plugin::~chain_api_plugin(){}

void chain_api_plugin::set_program_options(options_description&, options_description&) {}
void chain_api_plugin::plugin_initialize(const variables_map&) {}

#define CALL(api_name, api_handle, api_namespace, call_name, http_response_code) \
{std::string("/v1/" #api_name "/" #call_name), \
   [this, api_handle](string, string body, url_response_callback cb) mutable { \
          try { \
             if (body.empty()) body = "{}"; \
             auto result = api_handle.call_name(fc::json::from_string(body).as<api_namespace::call_name ## _params>()); \
             cb(http_response_code, fc::json::to_string(result)); \
          } catch (chain::tx_missing_sigs& e) { \
             error_results results{401, "UnAuthorized", e.to_string()}; \
             cb(401, fc::json::to_string(results)); \
          } catch (chain::tx_duplicate& e) { \
             error_results results{409, "Conflict", e.to_string()}; \
             cb(409, fc::json::to_string(results)); \
          } catch (chain::transaction_exception& e) { \
             error_results results{400, "Bad Request", e.to_string()}; \
             cb(400, fc::json::to_string(results)); \
          } catch (fc::eof_exception& e) { \
             error_results results{400, "Bad Request", e.to_string()}; \
             cb(400, fc::json::to_string(results)); \
             elog("Unable to parse arguments: ${args}", ("args", body)); \
          } catch (fc::exception& e) { \
             error_results results{500, "Internal Service Error", e.to_detail_string()}; \
             cb(500, fc::json::to_string(results)); \
             elog("Exception encountered while processing ${call}: ${e}", ("call", #api_name "." #call_name)("e", e)); \
          } \
       }}

#define CHAIN_RO_CALL(call_name, http_response_code) CALL(chain, ro_api, chain_apis::read_only, call_name, http_response_code)
#define CHAIN_RW_CALL(call_name, http_response_code) CALL(chain, rw_api, chain_apis::read_write, call_name, http_response_code)

void chain_api_plugin::plugin_startup() {
   ilog( "starting chain_api_plugin" );
   my.reset(new chain_api_plugin_impl(app().get_plugin<chain_plugin>().chain()));
   auto ro_api = app().get_plugin<chain_plugin>().get_read_only_api();
   auto rw_api = app().get_plugin<chain_plugin>().get_read_write_api();

   app().get_plugin<http_plugin>().add_api({
      CHAIN_RO_CALL(get_info, 200),
      CHAIN_RO_CALL(get_block, 200),
      CHAIN_RO_CALL(get_block_detail, 200),
      CHAIN_RO_CALL(get_account, 200),
      CHAIN_RO_CALL(get_code, 200),
      CHAIN_RO_CALL(get_table_rows, 200),
      CHAIN_RO_CALL(abi_json_to_bin, 200),
      CHAIN_RO_CALL(abi_bin_to_json, 200),
      CHAIN_RO_CALL(get_required_keys, 200),
      CHAIN_RW_CALL(push_block, 202),
      CHAIN_RW_CALL(push_transaction, 202),
      CHAIN_RW_CALL(push_transactions, 202)
   });
}

void chain_api_plugin::plugin_shutdown() {}

}

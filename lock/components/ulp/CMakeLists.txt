idf_component_register(SRCS "ulp.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES soc nvs_flash ulp driver)

set(ulp_app_name ulp_main)
set(ulp_s_sources "../../ulp/adc.S")
set(ulp_exp_dep_srcs "./ulp.cpp")

ulp_embed_binary(${ulp_app_name} "${ulp_s_sources}" "${ulp_exp_dep_srcs}")
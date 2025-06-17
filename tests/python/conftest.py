from ragger.conftest import configuration

###########################
### CONFIGURATION START ###
###########################

# You can configure optional parameters by overriding the value of ragger.configuration.OPTIONAL_CONFIGURATION
# Please refer to ragger/conftest/configuration.py for their descriptions and accepted values

# Define pytest markers
def pytest_configure(config):
    config.addinivalue_line(
        "markers",
        "active_test_scope: marks tests related to application name functionality",
    )
    # Add more markers here as needed


#########################
### CONFIGURATION END ###
#########################

# Pull all features from the base ragger conftest using the overridden configuration
pytest_plugins = ("ragger.conftest.base_conftest", )

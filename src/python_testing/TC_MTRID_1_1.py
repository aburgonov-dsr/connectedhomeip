import logging

from chip.clusters.Types import Nullable, NullValue
from matter_testing_support import (
    MatterBaseTest,
    async_test_body,
    default_matter_test_main,
)
from mobly import asserts
from TC_MeterIdentificationCommon import MeterIdAttributes, MeterIdSupporting


class TC_METERID_1_1(MatterBaseTest, MeterIdSupporting, MeterIdAttributes):

    # this function reads attributes from DUT
    async def read_meter_id_attrs(self, attribute):
        return await self.read_single_attribute(
            self.default_controller, self.dut_node_id, self.meter_id_endpoint, attribute
        )

    # this function is intended for printing info message with expected and actual results
    def logger(self, attr_name: str, expected_value, dut_value, expected_type):

        msg = (
            f"\n\t\t{attr_name.upper()} ATTRIBUTE EXPECTED VALUE: {expected_value}\n"
            f"\t\t{attr_name.upper()} ATTRIBUTE ACTUAL VALUE: {dut_value}\n"
            f"\t\t{attr_name.upper()} ATTRIBUTE EXPECTED TYPE: {type(expected_type)}\n"
            f"\t\t{attr_name.upper()} ATTRIBUTE ACTUAL TYPE: {type(dut_value)}"
        )
        logging.info(msg)

    # this function implements the main functionality of test
    async def main_steps(self, attribute_under_test):

        # iterating over all values for given attribute
        for value in attribute_under_test.values:

            self.set_own_attr_value(  # setting test value via named pipe
                f"{attribute_under_test.attribute_name}", f"{value}"
            )

            # reading value for given attribute from DUT
            attribute_from_dut = await self.read_meter_id_attrs(
                attribute_under_test.attribute
            )

            if value == "null":  # different check if null
                self.logger(
                    attribute_under_test.attribute_name,
                    value,
                    attribute_from_dut,
                    Nullable(),
                )
                asserts.assert_true(  # checking data type
                    (isinstance(attribute_from_dut, Nullable)),
                    "Data type of attribute is incorrect.",
                )
                asserts.assert_true(  # checking value
                    attribute_from_dut == NullValue, "Attribute has incorrect value."
                )
            else:
                self.logger(
                    attribute_under_test.attribute_name,
                    value,
                    attribute_from_dut,
                    attribute_under_test.data_type,
                )
                asserts.assert_true(
                    type(attribute_from_dut) is type(attribute_under_test.data_type),
                    "Data type of attribute is incorrect.",
                )
                asserts.assert_true(
                    (attribute_from_dut == value)
                    or (f'"{attribute_from_dut}"' == value),
                    "Attribute has incorrect value.",
                )

    # all code below corresponds to Matter TH
    def desc_TC_METERID_1_1(self) -> str:
        return "Testing Meter Identification Attribute: MeterType"

    def desc_TC_METERID_1_2(self) -> str:
        return "Testing Meter Identification Attribute: utilityName"

    def desc_TC_METERID_1_3(self) -> str:
        return "Testing Meter Identification Attribute: pointOfDelivery"

    def desc_TC_METERID_1_4(self) -> str:
        return "Testing Meter Identification Attribute: powerThreshold"

    def desc_TC_METERID_1_5(self) -> str:
        return "Testing Meter Identification Attribute: powerThresholdSource"

    @async_test_body
    async def test_TC_METERID_1_1(self):

        await self.main_steps(self.meter_type)

    @async_test_body
    async def test_TC_METERID_1_2(self):

        await self.main_steps(self.utility_name)

    @async_test_body
    async def test_TC_METERID_1_3(self):

        await self.main_steps(self.point_of_delivery)

    @async_test_body
    async def test_TC_METERID_1_4(self):

        await self.main_steps(self.power_threshold)

    @async_test_body
    async def test_TC_METERID_1_5(self):

        await self.main_steps(self.power_threshold_source)


if __name__ == "__main__":
    default_matter_test_main()

import subprocess
import time

import chip.clusters as Clusters
from chip.clusters.Types import NullValue
from chip.tlv import uint
from matter_testing_support import default_matter_test_main


# this class contains all additional functionality for testing
class MeterIdSupporting:

    pipe_path = None

    def _get_all_clusters_fifo_pipe(self):  # getting pipe name
        all_clusters_app_pipe = subprocess.run(
            "ls /tmp/ | grep chip_all_clusters_fifo",
            shell=True,
            stdout=subprocess.PIPE,
            encoding="utf-8",
        )
        if all_clusters_app_pipe.returncode == 1:
            raise RuntimeError("chip_all_clusters_fifo_XXX is absent!")
        else:
            self.pipe_path = "".join(["/tmp/", all_clusters_app_pipe.stdout.rstrip()])

    # writing our own value via pipe
    def set_own_attr_value(self, attr_name, attr_value):
        if self.pipe_path is None:
            self._get_all_clusters_fifo_pipe()
        with open(self.pipe_path, "w") as pipe:
            pipe.write(f'{{"Name":"MeterIdentification","{attr_name}":{attr_value}}}\n')
            time.sleep(1)


# this class contains description of all attributes for MeterID cluster
class MeterIdAttributes:

    meter_id_endpoint = 1  # cluster endpoint

    class MeterType:

        # generates values from self.values by request
        def generator(self):
            for item in self.values:
                yield item

        def __init__(self) -> None:
            self.attribute_name = "MeterType"
            self.attribute = Clusters.MeterIdentification.Attributes.MeterType
            self.values = [0, 1, 2, "null"]  # test values
            self.data_type = Clusters.MeterIdentification.Enums.MeterTypeEnum()
            self.attr_generator = self.generator()

        def get_value(self):  # returns values using generator
            return next(self.attr_generator)

        def regen_of_gen(self):  # resets generator to initial state
            self.attr_generator = self.generator()

    class UtilityName(MeterType):

        def __init__(self) -> None:
            super().__init__()
            self.attribute_name = "UtilityName"
            self.attribute = Clusters.MeterIdentification.Attributes.UtilityName
            self.values = ['""', '"a"', '"test-name"', '"It Company_Name1"', "null"]
            self.data_type = str()
            self.attr_generator = super().generator()

    class PointOfDelivery(MeterType):

        def __init__(self) -> None:
            super().__init__()
            self.attribute_name = "PointOfDelivery"
            self.attribute = Clusters.MeterIdentification.Attributes.PointOfDelivery
            self.values = ['""', '"a"', '"testData123"', '"It Company-Name1"', "null"]
            self.data_type = str()
            self.attr_generator = super().generator()

    class PowerThreshold(MeterType):

        def __init__(self) -> None:
            super().__init__()
            self.attribute_name = "PowerThreshold"
            self.attribute = Clusters.MeterIdentification.Attributes.PowerThreshold
            self.values = [0, 123456, 18446744073709551614, "null"]
            self.data_type = uint(1)
            self.attr_generator = super().generator()

    class PowerThresholdSource(MeterType):

        def __init__(self) -> None:
            super().__init__()
            self.attribute_name = "PowerThresholdSource"
            self.attribute = (
                Clusters.MeterIdentification.Attributes.PowerThresholdSource
            )
            self.values = [0, 1, 2, "null"]
            self.data_type = (
                Clusters.MeterIdentification.Enums.PowerThresholdSourceEnum()
            )
            self.attr_generator = super().generator()

    meter_type = MeterType()
    utility_name = UtilityName()
    point_of_delivery = PointOfDelivery()
    power_threshold = PowerThreshold()
    power_threshold_source = PowerThresholdSource()

    if __name__ == "__main__":
        default_matter_test_main()

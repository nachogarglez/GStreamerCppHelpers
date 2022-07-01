from conans import ConanFile, tools
import os


class GStreamerCppHelpersConan(ConanFile):
    name = "gstreamercpphelpers"
    description = "Headers to simplify GStreamer usage from C++."
    homepage = "https://github.com/nachogarglez/GStreamerCppHelpers"
    url = "https://github.com/nachogarglez/GStreamerCppHelpers"
    license = "LGPL-3.0"
    topics = ("gstreamer", "c++")
    no_copy_source = True
    settings = "os", "arch", "compiler", "build_type"
    generators = "cmake"

    @property
    def _source_subfolder(self):
        return os.path.join(self.source_folder, "source_subfolder")

    def validate(self):
        if self.settings.compiler.cppstd:
            tools.check_min_cppstd(self, 11)

    def source(self):
        tools.get(**self.conan_data["sources"][self.version], destination=self._source_subfolder, strip_root=True)

    def package(self):
        self.copy(pattern="LICENSE", dst="licenses", src=self._source_subfolder)
        self.copy("*.h", src=self._source_subfolder, dst=os.path.join("include"))

    def package_id(self):
        self.info.header_only()

    def package_info(self):
        self.cpp_info.includedirs.append(os.path.join("include"))

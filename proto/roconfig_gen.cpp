/*
    GSP for the Taurion blockchain game
    Copyright (C) 2019-2020  Autonomous Worlds Ltd

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "config.h"

#include "config.pb.h"

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include <cstdlib>
#include <fstream>
#include <iostream>

namespace
{

using google::protobuf::TextFormat;

constexpr const char* ROCONFIG_PROTO_TEXT = R"(
params:
{
    # The address configured here (for mainnet) is the premine address of Xaya
    # controlled by the Xaya team.  See also Xaya Core's src/chainparams.cpp.
    dev_addr: "DHy2615XKevE23LVRVZVxGeqxadRGyiFW4"

    god_mode: false

    burnsale_stages: { amount_sold: 10000000000 price_sat: 10000 }
    burnsale_stages: { amount_sold: 10000000000 price_sat: 20000 }
    burnsale_stages: { amount_sold: 10000000000 price_sat: 50000 }
    burnsale_stages: { amount_sold: 20000000000 price_sat: 100000 }
}testnet_merge:
  {
    params:
      {
        dev_addr: "dSFDAWxovUio63hgtfYd3nz3ir61sJRsXn"
      }
  }
)";

constexpr const char* ROCONFIG_PROTO_TEXT_REGTEST = R"(
# This gets merged in with the mainnet configuration, so we only need
# to specify parameters that actually differ.

params:
  {
    dev_addr: "dHNvNaqcD7XPDnoRjAoyfcMpHRi5upJD7p"
    god_mode: true
  }
)";

DEFINE_string (out_binary, "", "Output file for the binary data");
DEFINE_string (out_text, "", "Output file for the text data");

} // anonymous namespace

int
main (int argc, char** argv)
{
  google::InitGoogleLogging (argv[0]);
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  gflags::SetUsageMessage ("Generate roconfig protocol buffer");
  gflags::SetVersionString (PACKAGE_VERSION);
  gflags::ParseCommandLineFlags (&argc, &argv, true);

  LOG (INFO) << "Parsing hard-coded text proto...";
  pxd::proto::ConfigData pb;
  CHECK (TextFormat::ParseFromString (ROCONFIG_PROTO_TEXT, &pb));
  CHECK (TextFormat::ParseFromString (ROCONFIG_PROTO_TEXT_REGTEST,
                                      pb.mutable_regtest_merge ()));

  if (!FLAGS_out_binary.empty ())
    {
      LOG (INFO) << "Writing binary proto to output file " << FLAGS_out_binary;
      std::ofstream out(FLAGS_out_binary, std::ios::binary);
      CHECK (pb.SerializeToOstream (&out));
    }

  if (!FLAGS_out_text.empty ())
    {
      LOG (INFO) << "Writing text proto to output file " << FLAGS_out_text;
      std::ofstream out(FLAGS_out_text);
      google::protobuf::io::OstreamOutputStream zcOut(&out);
      CHECK (TextFormat::Print (pb, &zcOut));
    }

  google::protobuf::ShutdownProtobufLibrary ();
  return EXIT_SUCCESS;
}

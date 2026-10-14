// SPDX-License-Identifier: BSD-3-Clause-Clear
package vendor.qti.hardware.dsp;
import vendor.qti.hardware.dsp.DSPDomain;
import vendor.qti.hardware.dsp.FastrpcDev;
interface IDspManager {
    int closeSession(in DSPDomain domain);
    FastrpcDev openSession(in DSPDomain domain);
}

import { execFileSync } from "node:child_process";
import { createHash } from "node:crypto";
import { writeFile } from "node:fs/promises";
import { basename, join, resolve } from "node:path";

const [sdkArgument, outputArgument, version = "unknown", releaseDate] =
  process.argv.slice(2);
if (!sdkArgument || !outputArgument) {
  throw new Error("usage: generate-firmware-sbom.mjs <pico-sdk> <output> [version] [YYYY-MM-DD]");
}

const sdk = resolve(sdkArgument);
const output = resolve(outputArgument);
const components = [
  { name: "pico-sdk", path: sdk, license: "BSD-3-Clause" },
  { name: "btstack", path: join(sdk, "lib/btstack"), license: "LicenseRef-BTstack" },
  { name: "tinyusb", path: join(sdk, "lib/tinyusb"), license: "MIT" },
  { name: "cyw43-driver", path: join(sdk, "lib/cyw43-driver"), license: "LicenseRef-CYW43-RP" },
];

function git(path, ...args) {
  return execFileSync("git", ["-C", path, ...args], { encoding: "utf8" }).trim();
}

const packages = [];
for (const [index, component] of components.entries()) {
  const revision = git(component.path, "rev-parse", "HEAD");
  packages.push({
    SPDXID: `SPDXRef-Package-${index + 1}`,
    name: component.name,
    versionInfo: revision,
    downloadLocation: "NOASSERTION",
    filesAnalyzed: false,
    licenseConcluded: component.license,
    licenseDeclared: component.license,
    copyrightText: "NOASSERTION",
    externalRefs: [{
      referenceCategory: "PACKAGE-MANAGER",
      referenceType: "purl",
      referenceLocator: `pkg:generic/${component.name}@${revision}`,
    }],
  });
}

const identity = createHash("sha256")
  .update(JSON.stringify({ version, packages: packages.map(({ name, versionInfo }) => ({ name, versionInfo })) }))
  .digest("hex");
const created = releaseDate
  ? new Date(`${releaseDate}T00:00:00Z`)
  : new Date();
if (Number.isNaN(created.getTime())) throw new Error("invalid release date");
const createdText = created.toISOString().replace(/\.\d{3}Z$/, "Z");

const document = {
  spdxVersion: "SPDX-2.3",
  dataLicense: "CC0-1.0",
  SPDXID: "SPDXRef-DOCUMENT",
  name: `Red Monkey MPG-firmware-${version}`,
  documentNamespace: `https://red-monkey-mpg.local/spdx/${version}/${identity}`,
  creationInfo: {
    created: createdText,
    creators: ["Tool: Red Monkey MPG generate-firmware-sbom.mjs"],
  },
  packages,
  relationships: packages.map((pkg) => ({
    spdxElementId: "SPDXRef-DOCUMENT",
    relationshipType: "DESCRIBES",
    relatedSpdxElement: pkg.SPDXID,
  })),
  annotations: [{
    annotationDate: createdText,
    annotationType: "OTHER",
    annotator: "Tool: Red Monkey MPG generate-firmware-sbom.mjs",
    comment: `Generated from SDK tree ${basename(sdk)}. LicenseRef entries require review of the bundled license text before distribution.`,
  }],
};

await writeFile(output, `${JSON.stringify(document, null, 2)}\n`, { flag: "wx" });
console.log(`Wrote ${output}`);

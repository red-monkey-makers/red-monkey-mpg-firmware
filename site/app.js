const repository = "red-monkey-makers/red-monkey-mpg-firmware";
const releasesUrl = `https://github.com/${repository}/releases`;
const apiUrl = `https://api.github.com/repos/${repository}/releases?per_page=10`;

const firmware = {
  gamepad: { title: "Gamepad receiver", marker: "gamepad-receiver" },
  mobile: { title: "Mobile app receiver", marker: "mobile-receiver" }
};

let selected = "gamepad";
let currentRelease = null;

const cards = [...document.querySelectorAll("[data-firmware]")];
const tabs = [...document.querySelectorAll("[data-tab]")];
const downloadButton = document.querySelector("#download-button");
const assetName = document.querySelector("#asset-name");
const assetDigest = document.querySelector("#asset-digest");
const downloadTitle = document.querySelector("#download-title");
const releaseSummary = document.querySelector("#release-summary");
const previewNote = document.querySelector("#preview-note");

function selectFirmware(type, syncTab = true) {
  selected = type;
  cards.forEach((card) => {
    const active = card.dataset.firmware === type;
    card.classList.toggle("selected", active);
    card.setAttribute("aria-checked", String(active));
  });
  if (syncTab) selectTab(type);
  renderDownload();
}

function selectTab(type) {
  tabs.forEach((tab) => {
    const active = tab.dataset.tab === type;
    tab.setAttribute("aria-selected", String(active));
    document.querySelector(`#${tab.getAttribute("aria-controls")}`).hidden = !active;
  });
}

function renderDownload() {
  const choice = firmware[selected];
  downloadTitle.textContent = choice.title;
  const asset = currentRelease?.assets?.find((item) => item.name.includes(choice.marker) && item.name.endsWith(".uf2"));

  if (!asset) {
    assetName.textContent = currentRelease ? "This release does not contain the selected UF2." : "Finding the newest firmware release…";
    assetDigest.hidden = true;
    downloadButton.textContent = currentRelease ? "View releases" : "Preparing download…";
    downloadButton.href = releasesUrl;
    downloadButton.classList.toggle("disabled", !currentRelease);
    downloadButton.setAttribute("aria-disabled", String(!currentRelease));
    return;
  }

  assetName.textContent = asset.name;
  assetDigest.textContent = asset.digest ? `SHA-256  ${asset.digest.replace("sha256:", "")}` : "";
  assetDigest.hidden = !asset.digest;
  downloadButton.textContent = "Download selected UF2";
  downloadButton.href = asset.browser_download_url;
  downloadButton.classList.remove("disabled");
  downloadButton.removeAttribute("aria-disabled");
}

async function loadRelease() {
  try {
    const response = await fetch(apiUrl, { headers: { Accept: "application/vnd.github+json" } });
    if (!response.ok) throw new Error(`GitHub returned ${response.status}`);
    const releases = await response.json();
    currentRelease = releases.find((release) => !release.draft && release.assets.some((asset) => asset.name.includes("gamepad-receiver"))) || null;
    if (!currentRelease) throw new Error("No compatible release found");
    releaseSummary.textContent = `${currentRelease.tag_name}${currentRelease.prerelease ? " · preview release" : " · current release"}`;
    previewNote.hidden = !currentRelease.prerelease;
  } catch (error) {
    releaseSummary.textContent = "Release lookup unavailable · use the GitHub Releases page";
    currentRelease = null;
    downloadButton.textContent = "View GitHub releases";
    downloadButton.href = releasesUrl;
    downloadButton.classList.remove("disabled");
    downloadButton.removeAttribute("aria-disabled");
    console.warn("Could not load firmware release metadata", error);
  }
  renderDownload();
}

cards.forEach((card) => card.addEventListener("click", () => selectFirmware(card.dataset.firmware)));
tabs.forEach((tab) => tab.addEventListener("click", () => {
  selectTab(tab.dataset.tab);
  selectFirmware(tab.dataset.tab, false);
}));

document.querySelector("#year").textContent = new Date().getFullYear();
loadRelease();

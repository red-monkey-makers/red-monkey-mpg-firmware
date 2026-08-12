# Red Monkey MPG GitHub Pages site

This directory is a self-contained static end-user site. It uses plain HTML,
CSS, and JavaScript and requires no package installation or build step.

## Local preview

From the repository root:

```sh
python3 -m http.server 8000 --directory site
```

Then open <http://localhost:8000>.

## Publish with GitHub Pages

The repository's `.github/workflows/pages.yml` workflow uploads this directory
as the Pages artifact whenever `site/` changes on `main`. In the repository
settings, open **Pages** and set **Source** to **GitHub Actions**. Run the
**Deploy firmware website** workflow manually once if it did not start after
the first push.

The firmware chooser reads public release metadata from the GitHub API and
selects the newest published release containing the standardized gamepad and
mobile receiver assets. If that lookup is unavailable, it links users to the
repository's Releases page.

import alabaster

project = 'vren'
copyright = '2022, Lorenzo Rutayisire'
author = 'loryruta'

exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

html_theme_path = [alabaster.get_path()]
extensions = ['alabaster']
html_theme = 'alabaster'

html_show_sourcelink = False # Remove View page source

html_static_path = ['_static']

html_theme_options = {
    "github_user": "loryruta",
    'github_repo': "vren",
}

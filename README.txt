CALAIS

INTRO
=====

This module integrates Drupal with the Calais Web-Service. The web
service allows automated content term-extraction and tagging. It also 
provides an API in which any contributed module can interact with Calais.


INSTALLATION
============

1) Place this module directory into your Drupal modules directory.

2) Install the ARC2 RDF Parser.  Download it from http://arc.semsol.org/
   and extract the contents into calais/arc_rdf/arc2. More instructions are
   available in the arc_rdf/README.txt file.

3) Enable the ARC RDF, Calais API, and Calais module in Drupal, at:
   administration -> site configuration -> modules (admin/build/modules)

4) Add Calais API Key and tune other settings at:
   administration -> site configuration -> modules -> Calais
   (admin/settings/calais-api)


CREDITS
========
Written by
  - Irakli Nadareshvili <irakli at phase2technology dot com>
  - Frank Febbraro <frank at phase2technology dot com>
  
Sponsored by
  - Phase2 Technology <http://www.phase2technology.com>
  - ThomsonReuters <http://www.thomsonreuters.com/>
  
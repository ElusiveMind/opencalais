INTRO
=====

This module integrates Drupal with the Calais Web-Service. The web
service allows automated content term-extraction and tagging. It also 
provides an API in which any contributed module can interact with Calais.


INSTALLATION
============

1) You will need to install the ARC2 php library as a library on your drupal site.
To do that download ARC2 from github: https://github.com/semsol/arc2 and put the 
contents in sites/all/libraries/arc/ 

After this ARC2.php should live at sites/all/libraries/arc/ARC2.php

2) Place this module directory into your Drupal modules directory.

3) Enable the Calais API, and Calais module in Drupal, at:
   administration -> site configuration -> modules (admin/build/modules)

3) Obtain Calais API from their website: 
   https://iameui-eagan-prod.thomsonreuters.com/iamui/UI/createUser

4) Add Calais API Key and tune other settings at:
   administration -> site configuration -> modules -> Calais
   (admin/settings/calais-api)
   
5) Set Up tagging on your content types as desired by visiting
<yoursitename.com>/admin/structure/types/manage/<content-type-name>/opencalais_fields


There are some reports that OpenCalais processing can use a lot of memory 
(the RDf data returned can be quite large)  Consider 64MB the minimum PHP
memory allocation needed. You may need more depending on what you have
installed and enabled.

CREDITS
========
Written by
  - Irakli Nadareshvili - irakli at phase2technology dot com
  - Frank Febbraro - frank at phase2technology dot com

Maintained by:
  - Michel R. Bagnall - mrbagnall@icloud.com  

Sponsored by
  - Phase2 Technology <http://www.phase2technology.com>
  - ThomsonReuters <http://www.thomsonreuters.com/>
  

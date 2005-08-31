#!/bin/sh
# --------------------------------------------------------
# $Date: 2005/08/31 18:23:49 $
# $Revision: 1.1 $ yjogol
# Pruefung bei Argumentuebergaben
# --------------------------------------------------------

if [ "$#" -eq 0 ]
then
	echo "Der gepatchte WebServer laeuft nicht !!!"
	echo "Muss er aber fuer diese Funktion ..."
	echo "Siehe auch: Tools - Test ..."
	exit 1
fi

function mountMountPoint(id)
{
	NewWindow('/control/mountMountPoint?id='+id, 'mount', '200', '200', 'no', '5000');
}

function unmountMountPoint(id)
{
	NewWindow('/control/unmountMountPoint?id='+id, 'unmount', '200', '200', 'no', '5000');
}

function deleteMountPoint(id)
{
	NewWindow('/control/removeMountPoint?id='+id, 'removeMountPoint', '200', '200', 'no', '5000');
}

function changeMountPoint(id)
{
	NewWindow('/control/editMountPointWindow?id='+id, 'editMountPoint', '780', '400', 'no');
}

function addMountPoint(id)
{
	NewWindow('/control/addMountPointWindow?id='+id, 'addMountPoint', '780', '400', 'no');
}

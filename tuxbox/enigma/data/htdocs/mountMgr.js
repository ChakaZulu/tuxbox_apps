function mountMountPoint(id)
{
	NewWindow('/control/mountMountPoint?id='+id, 'mount', '200', '200', 'no', '5000');
}

function unmountMountPoint(id)
{
	NewWindow('/control/unmountMountPoint?id='+id, 'unmount', '200', '200', 'no', '5000');
}

function removeMountPoint(id)
{
	NewWindow('/control/removeMountPoint?id='+id, 'removeMountPoint', '200', '200', 'no', '5000');
}

function showEditMountPointWindow(id)
{
	NewWindow('/control/editMountPointWindow?id='+id, 'editMountPoint', '780', '500', 'no');
}

function showAddMountPointWindow(id)
{
	NewWindow('/control/addMountPointWindow?id='+id, 'addMountPoint', '780', '500', 'no');
}

// SPDX-License-Identifier: MIT
// Copyright (c) 2024, Oleksandr Koval

$(function () {
    $.get('/version_selector.html', function (data) {
        // Inject version selector HTML into the page
        $('#projectnumber').html(data);

        // Event listener to handle version selection
        document.getElementById('versionSelector').addEventListener('change', function () {
            var selectedVersion = this.value;
            window.location.href = '/' + selectedVersion + '/index.html';

        });

        // Set the selected option based on the current version
        var currentVersion = window.location.pathname.split('/')[1];
        $('#versionSelector').val(currentVersion);
    });
});

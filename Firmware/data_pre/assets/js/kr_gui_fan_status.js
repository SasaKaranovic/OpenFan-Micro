// When page is loaded
$(function() {
    update_value_slider();
    get_controller_config();

    $("#fan_value").on("input change", function() {
        update_value_slider();
    } );
    $("#update_fan").on("click", function() {
        var percent = $("#fan_value").val();
        update_fan(0, percent);
    } );
    $("#btn-led-on").on("click", function() {
        configure_led(true);
    } );
    $("#btn-led-off").on("click", function() {
        configure_led(false);
    } );
    $("#confirm_5v_switch").on("click", function() {
        configure_fan(false);
    } );
    $("#confirm_12v_switch").on("click", function() {
        configure_fan(true);
    } );
    $("#deviceName").on("dblclick", function() {
        rename_device();
    } );

    // Periodic fan RPM update
    const interval = setInterval(function() {
       gui_update_fan_status();
     }, 1000);
});


function gui_update_fan_status()
{
    var url = '/api/v0/fan/status';
    var jqxhr = $.getJSON(url, function() {
    })
      .done(function(e) {
        $("#fan-0-rpm").text(e['data']['rpm']);
      })
      .fail(function(e) {
        $("#fan_0_rpm").text('ERR RPM');
      });
}

function update_fan(fan, value)
{
    value = parseInt((value), 10);
    var url = '/api/v0/fan/'+ fan + '/set?value='+value;

    console.log("Sending request to: "+ url);
    var jqxhr = $.get( url, function() {
    })
      .done(function(e) {
        console.log(e);
      })
      .fail(function(e) {
        console.log(e);
      });

}

function rename_device()
{
    let name = prompt("New name for this device (max 12 characters)", "MyFan");
    var hitCancel = !(name != "" && name !== null);

    if (hitCancel)
    {
        return;
    }

    var url = '/api/v0/openfan/name/set?name='+name;

    console.log("Sending request to: "+ url);
    var jqxhr = $.get( url, function() {
    })
      .done(function(e) {
        console.log(e);
        $('#deviceName').text("Renamed. Rebooting to apply... please wait")
        setTimeout(function() {
            location.reload();
        }, 5000);
      })
      .fail(function(e) {
        console.log(e);
        alert("Failed to update fan voltage. Please check console for detailed error.");
      });

}

function configure_led(enabled)
{
    var url = '/api/v0/led/disable';
    if (enabled)
    {
        url = '/api/v0/led/enable';
    }
    else
    {
        url = '/api/v0/led/disable';
    }

    console.log("Sending request to: "+ url);
    var jqxhr = $.get( url, function() {
    })
      .done(function(e) {
        console.log(e);
        get_controller_config();
      })
      .fail(function(e) {
        console.log(e);
        alert("Failed to update fan voltage. Please check console for detailed error.");
      });

}

function configure_fan(enable_12v)
{
    var url = '/api/v0/fan/voltage/low';
    if (enable_12v)
    {
        url = '/api/v0/fan/voltage/high?confirm=true';
    }
    else
    {
        url = '/api/v0/fan/voltage/low?confirm=true';
    }

    console.log("Sending request to: "+ url);
    var jqxhr = $.get( url, function() {
    })
      .done(function(e) {
        console.log(e);
        get_controller_config();
      })
      .fail(function(e) {
        console.log(e);
        alert("Failed to update fan voltage. Please check console for detailed error.");
      });

}

function update_value_slider()
{
    var fan_value = $("#fan_value").val();
    $('label[for="fan_value"]').text('Fan PWM: '+ fan_value +'%');
}

function get_controller_config()
{
    var url = '/api/v0/openfan/status';
    var jqxhr = $.getJSON(url, function() {
    })
      .done(function(e) {
        console.log(e);
        let fan_is_12v = (/true/).test(e['data']['fan_is_12v']);
        let act_led_enabled = (/true/).test(e['data']['act_led_enabled']);

        if (fan_is_12v)
        {
            $('#fan_voltage').text('12');
        }
        else
        {
            $('#fan_voltage').text('5');
        }

        if (act_led_enabled)
        {
            $('#led_status').text('blinking');
        }
        else
        {
            $('#led_status').text('off');
        }

      })
      .fail(function(e) {
        console.log(e);
        console.log("Failed to retrieve controller status!");
      });
}

